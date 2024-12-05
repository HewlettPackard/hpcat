/**
* (C) Copyright 2024 Hewlett Packard Enterprise Development LP
*
* Permission is hereby granted, free of charge, to any person obtaining a
* copy of this software and associated documentation files (the "Software"),
* to deal in the Software without restriction, including without limitation
* the rights to use, copy, modify, merge, publish, distribute, sublicense,
* and/or sell copies of the Software, and to permit persons to whom the
* Software is furnished to do so, subject to the following conditions:
*
* The above copyright notice and this permission notice shall be included
* in all copies or substantial portions of the Software.
*
* THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
* IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
* FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT.  IN NO EVENT SHALL
* THE AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR
* OTHER LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE,
* ARISING FROM, OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR
* OTHER DEALINGS IN THE SOFTWARE.
******************************************************************************
* hpcat: display NUMA and CPU affinities in the context of HPC applications
*
* URL       https://github.com/HewlettPackard/hpcat
******************************************************************************/

#define _GNU_SOURCE
#include <dlfcn.h>
#include <unistd.h>
#include <string.h>
#include <stdio.h>
#include <limits.h>
#include <stdlib.h>
#include <omp.h>
#include <libgen.h>

#include "hpcat.h"
#include "settings.h"
#include "output.h"

#define FATAL(...)                              \
        do {                                    \
                fprintf(stderr, __VA_ARGS__);   \
                fflush(stderr);                 \
                exit(1);                        \
        } while(0)

#define MPI_CHECK(x)                                                                       \
        do {                                                                               \
            const int err=x;                                                               \
            if (x!=MPI_SUCCESS) {                                                          \
                int len; char estr[MPI_MAX_ERROR_STRING];                                  \
                MPI_Error_string(err, estr, &len);                                         \
                FATAL("Error: MPI error %d at %d: %s. Exiting.\n", err, __LINE__, estr);   \
            }                                                                              \
        } while(0)

hwloc_topology_t topology;

/**
 * Retrieve CPU core and NUMA node affinities
 *
 * @param   affinity[out]    Affinity structure
 */
void get_cpu_numa_affinity(Affinity *affinity)
{
    /* Retrieving CPU affinity */
    affinity->cpu_affinity = hwloc_bitmap_alloc();
    if (affinity->cpu_affinity == NULL)
        FATAL("Error: unable to allocate a hwloc bitmap for CPU affinity. Exiting.\n");

    if (hwloc_get_cpubind(topology, affinity->cpu_affinity,  HWLOC_CPUBIND_THREAD) != 0)
        FATAL("Error: unable to retrieve CPU binding with hwloc: %s. Exiting.\n", strerror(errno));

    /* Retrieving NUMA affinity */
    affinity->numa_affinity = hwloc_bitmap_alloc();
    if (affinity->numa_affinity == NULL)
        FATAL("Error: unable to allocate a hwloc bitmap for NUMA affinity. Exiting.\n");

    const int depth = hwloc_get_type_depth(topology, HWLOC_OBJ_NUMANODE);
    const int num_nodes = hwloc_get_nbobjs_by_depth(topology, depth);

    for (int i = 0; i < num_nodes; i++)
    {
        hwloc_obj_t node = hwloc_get_obj_by_depth(topology, depth, i);
        if (hwloc_bitmap_intersects(affinity->cpu_affinity, node->cpuset))
            hwloc_bitmap_set(affinity->numa_affinity, i);
    }
}

/**
 * Retrieve accelerator count, PCIe addresses and NUMA node affinities using dyn library
 *
 * @param   check_lib[in]    Path to a library to check if the software stack of a gpu type is available
 * @param   dyn_module[in]   Dynamic library to load if the software stack is available
 * @param   task[out]        Task handle to store information
 */
void try_get_accel_info(const char *check_lib, const char *dyn_module, Task *task)
{
    /* Check if accelerator library is installed */
    void *handle = dlopen(check_lib, RTLD_LAZY);
    if (handle == NULL)
        return;

    dlclose(handle);

    /* Get directory path of this binary */
    char buf[PATH_MAX], *lib_path;
    readlink("/proc/self/exe", buf, PATH_MAX - 1);
    lib_path = dirname(buf);

    /* Get full path where the module is supposed to be stored */
    char dynlib_path[PATH_MAX];
    strncpy(dynlib_path, lib_path, PATH_MAX - 1);
    strncat(dynlib_path, dyn_module, PATH_MAX - 1 - strlen(dyn_module));

    /* Load dynamic module */
    handle = dlopen(dynlib_path, RTLD_LAZY);
    if (handle == NULL)
        return;

    /* Retrieve accelerator information with the dynamic library */
    char *error;
    int (*device_count)(void) = dlsym(handle, "hpcat_accel_count");
    if ((error = dlerror()) != NULL)
        FATAL("Error: unable to load hpcat_accel_count with dyn library %s: %s. Exiting.\n", dyn_module, error);

    const int count = device_count();
    if (count <= 0)
        goto exit;
    task->accel.num_accel += count;

    int (*pciaddr_list_str)(char *buff, const int max_buff_size) = dlsym(handle, "hpcat_accel_pciaddr_list_str");
    if ((error = dlerror()) != NULL)
        FATAL("Error: unable to load hpcat_accel_pciaddr_list_str with dyn library %s: %s. Exiting.\n", dyn_module, error);
    pciaddr_list_str(task->accel.pciaddr, STR_MAX);

    int (*numa_list_str)(hwloc_bitmap_t numa) = dlsym(handle, "hpcat_accel_numa_list_str");
    if ((error = dlerror()) != NULL)
        FATAL("Error: unable to load hpcat_accel_numa_list_str with dyn library %s: %s. Exiting.\n", dyn_module, error);
    numa_list_str(task->accel.numa_affinity);

exit:
    dlclose(handle);
}

/**
 * Parse buffer to find data after the needle and before a comma
 *
 * @param   dest[out]      Where to store parsed data
 * @param   max_len[in]    Max length of the destination buffer
 * @param   buf[in]        Buffer to parse
 * @param   needle[in]     Keyword to find in the input buffer
 */
const char *parse_buffer(char *dest, const int max_len, const char *buf, const char *needle)
{
    const char *pos = strstr(buf, needle);
    if (pos == NULL)
        return NULL;

    pos += strlen(needle);
    const char *end_pos = strstr(pos, ",");
    const int len = end_pos - pos;

    if (len >= max_len)
        return NULL;

    strncpy(dest, pos, len);
    return end_pos;
}

/**
 * Retrieve NIC information by activating verbose flag before calling MPI_Init
 */
void MPI_Init_verbose(Task *task, int *nargs, char **args[])
{
    char buf[STR_MAX];
    int stderr_bk = dup(fileno(stderr));
    if (stderr_bk == -1)
        FATAL("Error: dup unable to duplicate stderr: %s\n", strerror(errno));

    int pipefd[2];
    if (pipe2(pipefd, 0) == -1)
        FATAL("Error: unable to create a pipe: %s\n", strerror(errno));

    /* sterr will now go to the pipe */
    if (dup2(pipefd[1], fileno(stderr)) == -1)
        FATAL("Error: dup2 unable to duplicate stderr: %s\n", strerror(errno));

    /* Enabling MPI verbosity */
    setenv("MPICH_OFI_NIC_VERBOSE", "2", 1);

    MPI_Init(nargs, args);
    fflush(stderr);

    close(pipefd[1]);

    /* Restoring stderr */
    if (dup2(stderr_bk, fileno(stderr)) == -1)
        FATAL("Error: unable to restore file descriptor on stderr: %s\n", strerror(errno));

    read(pipefd[0], buf, STR_MAX);

    /* Check MPICH NIC selection */
    memset(&task->nic, 0, sizeof(Nic));
    const char *nic_pos = parse_buffer(task->nic.name, NIC_STR_MAX, buf, ", domain_name=");
    if (nic_pos != NULL)
    {
        char numa_str[NIC_STR_MAX] = { 0 };
        const char *numa_pos = parse_buffer(numa_str, NIC_STR_MAX, nic_pos, "numa_node=");
        if (numa_pos != NULL)
        {
            task->nic.numa_affinity = hwloc_bitmap_alloc();
            hwloc_bitmap_set(task->nic.numa_affinity, atoi(numa_str));
        }
        task->nic.num_nic = 1;
    }

    unsetenv("MPICH_OFI_NIC_VERBOSE");
}

/**
 * Discard output in stdout with MPI_Finalize
 */
void MPI_Finalize_noverbose(void)
{
    int stdout_bk = dup(fileno(stdout));
    if (stdout_bk == -1)
        FATAL("Error: dup unable to duplicate stdout: %s\n", strerror(errno));

    int pipefd[2];
    if (pipe2(pipefd, 0) == -1)
        FATAL("Error: unable to create a pipe: %s\n", strerror(errno));

    if (dup2(pipefd[1], fileno(stdout)) == -1)
        FATAL("Error: dup2 unable to duplicate stdout: %s\n", strerror(errno));

    MPI_Finalize();
    fflush(stdout);

    close(pipefd[1]);

    /* Restoring stdout */
    if (dup2(stdout_bk, fileno(stdout)) == -1)
        FATAL("Error: unable to restore file descriptor on stdout: %s\n", strerror(errno));
}

/**
 * Retrieve MPI, OMP and accelerator based information
 *
 * @param   hpcat    Application handle
 */
void hpcat_init(Hpcat *hpcat)
{
    /* Retrieving MPI info */
    Task *task = &hpcat->task;
    MPI_CHECK( MPI_Comm_size(MPI_COMM_WORLD, &hpcat->num_tasks) );
    MPI_CHECK( MPI_Comm_rank(MPI_COMM_WORLD, &task->id) );

    /* Retrieving MPI distribution and version, only keep first line */
    int resultlen;
    MPI_CHECK( MPI_Get_library_version(hpcat->mpi_version, &resultlen) );
    char* ptr = strstr(hpcat->mpi_version, "\n");
    if (ptr != NULL)
        ptr[0] = '\0';

    /* Retrieving hostname */
    if (gethostname(task->hostname, HOST_NAME_MAX) != 0)
        FATAL("Error: unable to get host name: %s. Exiting\n", strerror(errno));

    /* Loading hwloc topology */
    if (hwloc_topology_init(&topology) != 0)
        FATAL("Error: unable to initialize hwloc. Exiting.\n");

    if (hwloc_topology_load(topology) != 0)
        FATAL("Error: unable to load hwloc topology. Exiting.\n");

    /* Retrieving NUMA and CPU core affinities */
    get_cpu_numa_affinity(&task->affinity);

    memset(&task->accel, 0, sizeof(Accelerators));
    task->accel.numa_affinity = hwloc_bitmap_alloc();
    if (task->accel.numa_affinity == NULL)
        FATAL("Error: unable to allocate a hwloc bitmap. Exiting.\n");

    /* Checking if HIP is available, if so fetch information */
    try_get_accel_info("/opt/rocm/lib/libamdhip64.so", "/hpcathip.so", task);

    /* Checking if CUDA is available, if so fetch information */
    try_get_accel_info("/usr/lib64/libnvidia-ml.so", "/hpcatnvml.so", task);

    /* Retrieving OMP CPU affinities and thread IDs */
    #pragma omp parallel
    {
        const int thread_id = omp_get_thread_num();

        #pragma omp single
        {
            task->num_threads = omp_get_num_threads();
            task->threads = (Thread*)malloc(task->num_threads * sizeof(Thread));
        }

        Thread *thread = &task->threads[thread_id];
        thread->id = thread_id;
        get_cpu_numa_affinity(&thread->affinity);
    }
}

int main(int argc, char* argv[])
{
    /* Hide potential Cray warnings */
    setenv("OMP_WAIT_POLICY", "PASSIVE", 1);

    /* Disable GPU aware MPI to avoid having to link to GTL */
    unsetenv("MPICH_GPU_SUPPORT_ENABLED");

    Hpcat hpcat = { 0 };

    /* Retrieving user defined parameters passed as arguments */
    hpcat_settings_init(argc, argv, &hpcat.settings);

    /* Initializing MPI in verbose mode */
    MPI_Init_verbose(&hpcat.task, &argc, &argv);

    /* Retrieve MPI, OMP and accelerator based details */
    hpcat_init(&hpcat);

    /* Mapping between hostname and ranks */
    char map[hpcat.num_tasks][HOST_NAME_MAX];
    strncpy(map[hpcat.task.id], hpcat.task.hostname, HOST_NAME_MAX - 1);
    MPI_CHECK( MPI_Allgather(MPI_IN_PLACE, 0, 0, map[0], HOST_NAME_MAX, MPI_CHAR, MPI_COMM_WORLD) );

    /* List of nodes */
    char hostnames[hpcat.num_tasks][HOST_NAME_MAX];
    strncpy(hostnames[0], map[0], HOST_NAME_MAX - 1);
    hpcat.num_nodes = 1;
    for (int i = 1; i < hpcat.num_tasks; i++)
    {
        int j;
        for (j = 0; j < hpcat.num_nodes; j++)
        {
            if (strncmp(map[i], hostnames[j], HOST_NAME_MAX - 1) == 0)
                break;
        }

        if (j == hpcat.num_nodes)
        {
            strncpy(hostnames[hpcat.num_nodes], map[i], HOST_NAME_MAX - 1);
            hpcat.num_nodes++;
        }
    }

    /* Disable NIC affinity if only one node */
    if (hpcat.num_nodes == 1)
        hpcat.settings.enable_nic = false;

    /* Reordered list of ranks ( all ranks on a node before going to the next one) */
    int reordered_ranks[hpcat.num_tasks];
    int pos = 0;
    hpcat.task.is_first_node_rank = false;
    for (int i = 0; i < hpcat.num_nodes; i++)
    {
        bool is_first_node_rank = true;
        for (int j = 0; j < hpcat.num_tasks; j++)
        {
            if (strncmp(hostnames[i], map[j], HOST_NAME_MAX - 1) == 0)
            {
                if (is_first_node_rank)
                {
                    if (j == hpcat.task.id)
                        hpcat.task.is_first_node_rank = true;

                    is_first_node_rank = false;
                }
                reordered_ranks[pos] = j;
                pos++;
            }
        }
    }

    hpcat.task.is_first_rank = (hpcat.task.id == reordered_ranks[0]);
    hpcat.task.is_last_rank = (hpcat.task.id == reordered_ranks[hpcat.num_tasks - 1]);

    /* Sync to display output in sequence */
    if (!hpcat.task.is_first_rank)
    {
        MPI_Status status; int flag;
        MPI_CHECK( MPI_Recv(&flag, 1, MPI_INT, reordered_ranks[hpcat.task.id - 1], 100, MPI_COMM_WORLD, &status) );
    }

    switch (hpcat.settings.output_type)
    {
        case STDOUT:
            hpcat_display_stdout(&hpcat);
            break;
        case YAML:
            hpcat_display_yaml(&hpcat);
            break;
    }

    usleep(10); // Wait output get aggregated by the job manager

    if (!hpcat.task.is_last_rank)
    {
        MPI_CHECK( MPI_Send(&hpcat.task.id, 1, MPI_INT, reordered_ranks[hpcat.task.id + 1], 100, MPI_COMM_WORLD) );
    }

    /* Clean up */
    hwloc_topology_destroy(topology);

    MPI_Finalize_noverbose();
    return 0;
}
