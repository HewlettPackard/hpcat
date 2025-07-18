/**
* (C) Copyright 2025 Hewlett Packard Enterprise Development LP
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

#include <dlfcn.h>
#include <unistd.h>
#include <string.h>
#include <stdio.h>
#include <limits.h>
#include <stdlib.h>
#include <omp.h>
#include <libgen.h>
#include <ifaddrs.h>
#include <net/if.h>
#include <netpacket/packet.h>
#include <dirent.h>

#include "hpcat.h"
#include "common.h"
#include "settings.h"
#include "output.h"
#include "hint.h"

#define AMA_GROUP_SHIFTS   11 /* Position of Dragonfly group id in a Slingshot MAC address */
#define FABRIC_GROUPS_MAX 256

#define MPI_CHECK(x)                                                                       \
        do {                                                                               \
            const int err = x;                                                             \
            if (x != MPI_SUCCESS) {                                                        \
                int len; char estr[MPI_MAX_ERROR_STRING];                                  \
                MPI_Error_string(err, estr, &len);                                         \
                FATAL("Error: MPI error %d at %d: %s. Exiting.\n", err, __LINE__, estr);   \
            }                                                                              \
        } while(0)

hwloc_topology_t topology;

static void serialize_bitmap(Bitmap *bitmap, hwloc_bitmap_t tmp)
{
    bitmap->num_ulongs = hwloc_bitmap_nr_ulongs(tmp);
    if (bitmap->num_ulongs <= 0)
        FATAL("Error: unable to evaluate qty of ulongs needed to serialize the bitmap. Exiting.\n");

    if (bitmap->num_ulongs > BITMAP_ULONGS_MAX)
        FATAL("Error: bitmap size is too small. Exiting.\n");

    if (hwloc_bitmap_to_ulongs(tmp, bitmap->num_ulongs, bitmap->ulongs) != 0)
        FATAL("Error: unable to convert bitmap to ulongs. Exiting.\n");
}

static void serialize_cpu_bitmap(CPUBitmap *bitmap, hwloc_bitmap_t tmp)
{
    bitmap->num_ulongs = hwloc_bitmap_nr_ulongs(tmp);
    if (bitmap->num_ulongs <= 0)
        FATAL("Error: unable to convert cpu bitmap to ulongs. Exiting.\n");

    if (bitmap->num_ulongs > BITMAP_CPU_ULONGS_MAX)
        FATAL("Error: cpu bitmap size is too small. Exiting.\n");

    if (hwloc_bitmap_to_ulongs(tmp, bitmap->num_ulongs, bitmap->ulongs) != 0)
        FATAL("Error: unable to convert cpu bitmap to ulongs. Exiting.\n");
}

static void emulate_mpich_ofi_nic_policy_gpu(Hpcat *hpcat, Task *task, void* dlopen_handle)
{
    /* XXX: When using Slingshot with Cray MPICH, setting the environment variable
     * MPICH_OFI_NIC_POLICY to GPU enables this function to emulate NIC affinity
     * to match GPU NUMA affinity. This Cray MPICH feature requires certain
     * libraries that are not linked during compilation to maintain modularity.
     */

    const bool nic_is_cxi = (strstr(task->nic.name, "cxi") != NULL);

    /* Only display NIC locality display is emulation is possible */
    task->nic.numa_affinity = -1;
    task->nic.name[0] = '\0';

    /* Ensure a Slingshot interface is used, otherwise skip emulation */
    if (!nic_is_cxi)
        return;

    /* Get the NUMA locality of the first GPU */
    int (*numa_first)(void) = dlsym(dlopen_handle, "hpcat_accel_numa_first");
    char *error;
    if ((error = dlerror()) != NULL)
        FATAL("Error: unable to load hpcat_accel_numa_first: %s. Exiting.\n", error);

    const int gpu_numa = numa_first();

    /* Try now to match the NUMA locality to a Slingshot interface */
    DIR *dir;
    struct dirent *entry;
    char path[PATH_MAX], numa_node_path[PATH_MAX];
    int nic_numa;

    if ((dir = opendir("/sys/class/net/")) == NULL)
        FATAL("Error: unable to opendir /sys/class/net. Exiting.\n");

    /* Iterate through each network interface */
    while ((entry = readdir(dir)) != NULL)
    {
        if (strstr(entry->d_name, "hsn") == NULL)
            continue;

        snprintf(path, sizeof(path), "/sys/class/net/%s/device/", entry->d_name);
        snprintf(numa_node_path, sizeof(numa_node_path), "%s/numa_node", path);

        FILE *numa_file = fopen(numa_node_path, "r");
        if (numa_file != NULL)
        {
            fscanf(numa_file, "%d", &nic_numa);
            fclose(numa_file);
        }

        if (nic_numa == gpu_numa)
        {
            sprintf(task->nic.name, "cxi%c", entry->d_name[strlen(entry->d_name) - 1]);
            task->nic.numa_affinity = gpu_numa;
            break;
        }
    }

    closedir(dir);
}

/**
 * Retrieve CPU core and NUMA node affinities
 *
 * @param   affinity[out]    Affinity structure
 */
void get_cpu_numa_affinity(Affinity *affinity)
{
    /* Retrieving CPU (hardware thread) affinity */
    hwloc_bitmap_t hw_thread_affinity = hwloc_bitmap_alloc();
    if (hw_thread_affinity == NULL)
        FATAL("Error: unable to allocate a hwloc bitmap for CPU (hardware thread) affinity. Exiting.\n");

    if (hwloc_get_cpubind(topology, hw_thread_affinity, HWLOC_CPUBIND_THREAD) != 0)
        FATAL("Error: unable to retrieve CPU binding with hwloc: %s. Exiting.\n", strerror(errno));

    /* Retrieving CPU core (first hardware thread) affinity */
    hwloc_bitmap_t core_affinity = hwloc_bitmap_alloc();
    core_affinity = hwloc_bitmap_alloc();
    if (core_affinity == NULL)
        FATAL("Error: unable to allocate a hwloc bitmap for CPU core affinity. Exiting.\n");

    const int depth_core = hwloc_get_type_depth(topology, HWLOC_OBJ_CORE);
    const int num_cores = hwloc_get_nbobjs_by_depth(topology, depth_core);

    for (int i = 0; i < num_cores; i++)
    {
        hwloc_obj_t core = hwloc_get_obj_by_depth(topology, depth_core, i);
        if (hwloc_bitmap_intersects(hw_thread_affinity, core->cpuset))
            hwloc_bitmap_set(core_affinity, core->first_child->os_index);
    }

    /* Retrieving NUMA affinity */
    hwloc_bitmap_t numa_affinity = hwloc_bitmap_alloc();
    if (numa_affinity == NULL)
        FATAL("Error: unable to allocate a hwloc bitmap for NUMA affinity. Exiting.\n");

    const int depth_node = hwloc_get_type_depth(topology, HWLOC_OBJ_NUMANODE);
    const int total_numa = hwloc_get_nbobjs_by_depth(topology, depth_node);

    for (int i = 0; i < total_numa; i++)
    {
        hwloc_obj_t node = hwloc_get_obj_by_depth(topology, depth_node, i);
        if (hwloc_bitmap_intersects(hw_thread_affinity, node->cpuset))
            hwloc_bitmap_set(numa_affinity, i);
    }

    /* Serialize bitmaps */
    serialize_cpu_bitmap(&affinity->hw_thread_affinity, hw_thread_affinity);
    serialize_cpu_bitmap(&affinity->core_affinity, core_affinity);
    serialize_bitmap(&affinity->numa_affinity, numa_affinity);

    /* Clean up */
    hwloc_bitmap_free(hw_thread_affinity);
    hwloc_bitmap_free(core_affinity);
    hwloc_bitmap_free(numa_affinity);
}

/**
 * Retrieve accelerator count, PCIe addresses and NUMA node affinities using dyn library
 *
 * @param   hpcat[in]        Application handle
 * @param   task[inout]      Task handle
 * @param   check_lib[in]    Path to a library to check if the software stack of a gpu type is available
 * @param   dyn_module[in]   Dynamic library to load if the software stack is available
 */
void try_get_accel_info(Hpcat *hpcat, Task *task, const char *check_lib, const char *dyn_module)
{
    /* Check if accelerator library is installed */
    void *handle = dlopen(check_lib, RTLD_LAZY);
    if (handle == NULL)
    {
        VERBOSE(hpcat, "Verbose: %s not found in the search path. Disabling %s.\n", check_lib, dyn_module);
        return;
    }

    dlclose(handle);

    /* Get directory path of this binary */
    char buf[PATH_MAX], *current_path;
    readlink("/proc/self/exe", buf, PATH_MAX - 1);
    current_path = dirname(buf);

    /* Get full path where the module is supposed to be stored */
    char dynlib_path[PATH_MAX];
    strncpy(dynlib_path, current_path, PATH_MAX - 1);
    strncat(dynlib_path, "/../lib/", PATH_MAX - 2);
    strncat(dynlib_path, dyn_module, PATH_MAX - 2 - strlen(dyn_module));

    /* Load dynamic module */
    handle = dlopen(dynlib_path, RTLD_LAZY);
    if (handle == NULL)
    {
        VERBOSE(hpcat, "Verbose: missing %s module.\n", dynlib_path);
        return;
    }

    /* Allocate temporary bitmaps */
    hwloc_bitmap_t numa_affinity = hwloc_bitmap_alloc();
    if (numa_affinity == NULL)
        FATAL("Error: unable to allocate a hwloc bitmap (numa_affinity). Exiting.\n");

    hwloc_bitmap_t visible_devices = hwloc_bitmap_alloc();
    if (visible_devices == NULL)
        FATAL("Error: unable to allocate a hwloc bitmap (visible_devices). Exiting.\n");

    /* Retrieve accelerator information with the dynamic library */
    char *error;
    int (*device_count)(void) = dlsym(handle, "hpcat_accel_count");
    if ((error = dlerror()) != NULL)
        FATAL("Error: unable to load hpcat_accel_count with dyn library %s: %s. Exiting.\n", dyn_module, error);

    const int count = device_count();
    if (count <= 0)
        goto exit;

    Accelerators *accel = &task->accel;
    accel->num_accel += count;

    int (*pciaddr_list_str)(char *buff, const int max_buff_size) = dlsym(handle, "hpcat_accel_pciaddr_list_str");
    if ((error = dlerror()) != NULL)
        FATAL("Error: unable to load hpcat_accel_pciaddr_list_str with dyn library %s: %s. Exiting.\n", dyn_module, error);
    pciaddr_list_str(accel->pciaddr, STR_MAX);

    int (*numa_bitmap)(hwloc_bitmap_t numa) = dlsym(handle, "hpcat_accel_numa_bitmap");
    if ((error = dlerror()) != NULL)
        FATAL("Error: unable to load hpcat_accel_numa_bitmap with dyn library %s: %s. Exiting.\n", dyn_module, error);
    if (numa_bitmap(numa_affinity) != 0)
        FATAL("Error: hpcat_accel_numa_bitmap with dyn library %s. Exiting.\n", dyn_module);

    int (*visible_bitmap)(hwloc_bitmap_t numa) = dlsym(handle, "hpcat_accel_visible_bitmap");
    if ((error = dlerror()) != NULL)
        FATAL("Error: unable to load hpcat_accel_visible_bitmap with dyn library %s: %s. Exiting.\n", dyn_module, error);
    if (visible_bitmap(visible_devices) != 0)
        FATAL("Error: hpcat_accel_visible_bitmap with dyn library %s. Exiting.\n", dyn_module);

    /* Serialize bitmaps */
    serialize_bitmap(&accel->numa_affinity, numa_affinity);
    serialize_bitmap(&accel->visible_devices, visible_devices);

    VERBOSE(hpcat, "Verbose: %s module enabled.\n", dyn_module);

    if (task->is_mpich_ofi_nic_policy_gpu)
         emulate_mpich_ofi_nic_policy_gpu(hpcat, task, handle);

exit:
    dlclose(handle);
    hwloc_bitmap_free(numa_affinity);
    hwloc_bitmap_free(visible_devices);
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
 *
 * @param   task[inout]   Task handle
 * @param   nargs[in]     Program argument count
 * @param   args[in]      Program argument vector
 */
void MPI_Init_verbose(Task *task, int *nargs, char **args[])
{
    char buf[STR_MAX];
    int stderr_bk = dup(fileno(stderr));
    if (stderr_bk == -1)
        FATAL("Error: dup unable to duplicate stderr: %s\n", strerror(errno));

    int pipefd[2];
    if (pipe(pipefd) == -1)
        FATAL("Error: unable to create a pipe: %s\n", strerror(errno));

    /* sterr will now go to the pipe */
    if (dup2(pipefd[1], fileno(stderr)) == -1)
        FATAL("Error: dup2 unable to duplicate stderr: %s\n", strerror(errno));

    /* XXX: Verify that MPICH_OFI_NIC_POLICY=GPU is disabled (emulated if set),
     * as this configuration disrupts the modular functionality of the tool.
     * Ensure that the tool operates correctly without this setting. */
    char *nic_policy = getenv("MPICH_OFI_NIC_POLICY");
    if (nic_policy != NULL && strstr(nic_policy, "GPU") != NULL)
    {
        unsetenv("MPICH_OFI_NIC_POLICY");
        task->is_mpich_ofi_nic_policy_gpu = true;
    }

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
            task->nic.numa_affinity = (char)atoi(numa_str);
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
    if (pipe(pipefd) == -1)
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
 * Retrieve fabric locality information
 *
 * @param   hpcat[inout]    Application handle
 * @param   task[inout]     Task handle
 */
void try_get_fabric_info(Hpcat *hpcat, Task *task)
{
    if (!hpcat->settings.enable_fabric)
        return;

    /* XXX: For now only detecting HPE Slingshot with Dragonfly topology.
     * The DragonFly group id can be found in the MAC address of a Slingshot NIC */
    struct ifaddrs *ifaddr, *ifa;
    if (getifaddrs(&ifaddr) == -1)
        FATAL("Error: unable to retrieve a list of network interfaces: %s\n", strerror(errno));

    int group_id = -1;
    for (ifa = ifaddr; ifa != NULL; ifa = ifa->ifa_next)
    {
        if (strstr(ifa->ifa_name, "hsn") == NULL) continue; /* Only selecting Slingshot interfaces */
        if (ifa->ifa_addr->sa_family != AF_PACKET) continue; /* Only interested in physical (MAC) addresses */

        struct sockaddr_ll *saddr = (struct sockaddr_ll*)ifa->ifa_addr;
        if (saddr->sll_halen == 6)
        {
            unsigned char *mac = saddr->sll_addr;
            unsigned int ama = (mac[3] << 16) | (mac[4] << 8) | mac[5];
            group_id = (ama >> AMA_GROUP_SHIFTS);
            break; /* Assuming all NICs are in the same group */
        }
        else
            FATAL("Error: unable to retrieve %s MAC address\n", ifa->ifa_name);
    }

    if (group_id >= 0)
    {
        task->fabric_group_id = group_id;
        VERBOSE(hpcat, "Verbose: Slingshot fabric detected and Dragonfly group id found.\n");
    }
    else
        hpcat->settings.enable_fabric = false;

    freeifaddrs(ifaddr);
}

/**
 * Retrieve MPI, OMP, fabric and accelerator based information
 *
 * @param   hpcat[inout]    Application handle
 * @param   task[inout]     Task handle
 */
void hpcat_init(Hpcat *hpcat, Task *task)
{
    /* Retrieving MPI info */
    MPI_CHECK( MPI_Comm_size(MPI_COMM_WORLD, &hpcat->num_tasks) );
    MPI_CHECK( MPI_Comm_rank(MPI_COMM_WORLD, &hpcat->id) );
    task->id = hpcat->id;

    /* Retrieving MPI distribution and version, only keep first line */
    int resultlen;
    MPI_CHECK( MPI_Get_library_version(hpcat->mpi_version, &resultlen) );
    char* ptr = strstr(hpcat->mpi_version, "\n");
    if (ptr != NULL)
        ptr[0] = '\0';

    /* Retrieving hostname */
    if (gethostname(task->hostname, HOST_NAME_MAX) != 0)
        FATAL("Error: unable to get host name: %s. Exiting\n", strerror(errno));

    /* Detect all cores regardless cgroups */
    setenv("HWLOC_THISSYSTEM", "1", 1);

    /* Loading hwloc topology */
    if (hwloc_topology_init(&topology) != 0)
        FATAL("Error: unable to initialize hwloc. Exiting.\n");

    /* XXX: Loading the topology is slow and appears to be a sequential operation for tasks
     * on the same node. To optimize this, only one rank per node now discovers the topology
     * and shares it with the other ranks on that node. */
    int node_rank, node_size;
    MPI_Comm node_comm;
    MPI_CHECK( MPI_Comm_split_type(MPI_COMM_WORLD, MPI_COMM_TYPE_SHARED, 0, MPI_INFO_NULL, &node_comm) );
    MPI_CHECK( MPI_Comm_rank(node_comm, &node_rank) );
    MPI_CHECK( MPI_Comm_size(node_comm, &node_size) );

    if (node_rank == 0) /* Local master load the topology */
    {
        char *buffer = NULL;
        int length = 0;

        if (hwloc_topology_load(topology) != 0)
            FATAL("Error: unable to load the hwloc topology. Exiting.\n");

        if (hwloc_topology_export_xmlbuffer(topology, &buffer, &length, 0) != 0)
            FATAL("Error: unable to export the hwloc topology. Exiting.\n");

        MPI_CHECK( MPI_Bcast(&length, 1, MPI_INT, 0, node_comm) );
        MPI_CHECK( MPI_Bcast(buffer, length, MPI_BYTE, 0, node_comm) );

        hwloc_free_xmlbuffer(topology, buffer);
    }
    else /* Other local ranks receive the topology */
    {
        int length = 0;
        MPI_CHECK( MPI_Bcast(&length, 1, MPI_INT, 0, node_comm) );

        char *buffer = (char *)malloc(length);
        if (buffer == NULL)
            FATAL("Error: unable to allocate hwloc buffer. Exiting.\n");

        MPI_CHECK( MPI_Bcast(buffer, length, MPI_BYTE, 0, node_comm) );

        if (hwloc_topology_set_xmlbuffer(topology, buffer, length) != 0)
            FATAL("Error: unable to import hwloc XML buffer. Exiting.\n");

        if (hwloc_topology_load(topology) != 0)
            FATAL("Error: unable to load the hwloc topology from XML buffer. Exiting.\n");

        free(buffer);
    }

    /* Retrieving NUMA and CPU core affinities */
    get_cpu_numa_affinity(&task->affinity);

    memset(&task->accel, 0, sizeof(Accelerators));

    /* Checking fabric locality */
    try_get_fabric_info(hpcat, task);

    /* Checking if HIP is available, if so fetch information */
    try_get_accel_info(hpcat, task, "libamdhip64.so", "libhpcathip.so");

    /* Checking if CUDA is available, if so fetch information */
    try_get_accel_info(hpcat, task, "libnvidia-ml.so", "libhpcatnvml.so");

    /* Checking if OneAPI Level Zero is available, if so fetch information */
    try_get_accel_info(hpcat, task, "libze_loader.so.1", "libhpcatze.so");

    /* Disable GPUs if no tasks can detect them */
    int accel_sum = 0;
    MPI_Allreduce(&task->accel.num_accel, &accel_sum, 1, MPI_INT, MPI_SUM, MPI_COMM_WORLD);
    hpcat->settings.enable_accel = (accel_sum > 0);

    VERBOSE(hpcat, "Verbose: %d visible accelerators (sum accross all tasks).\n", accel_sum);

    if (!hpcat->settings.enable_omp)
        return;

    /* Retrieving OMP CPU affinities and thread IDs */
    #pragma omp parallel
    {
        const int thread_id = omp_get_thread_num();

        #pragma omp single
        {
            task->num_threads = omp_get_num_threads();
            if (task->num_threads > THREADS_MAX)
                FATAL("Error: THREADS_MAX lower than amount of threads. Exiting.\n");
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
    Task task = { 0 };

    /* Retrieving user defined parameters passed as arguments */
    hpcat_settings_init(argc, argv, &hpcat.settings);

    /* Initializing MPI in verbose mode */
    MPI_Init_verbose(&task, &argc, &argv);

    /* Retrieve MPI, OMP and accelerator based details */
    hpcat_init(&hpcat, &task);

    /* Verify the binding and affinity, and determine whether hints should be displayed */
    hpcat_hint_task_check(&hpcat, &task);

    /* Mapping between hostname and ranks */
    char map[hpcat.num_tasks][HOST_NAME_MAX];
    strncpy(map[task.id], task.hostname, HOST_NAME_MAX - 1);
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

    /* Disable NIC and fabric affinity if only one node */
    if (hpcat.num_nodes == 1)
    {
        hpcat.settings.enable_nic = false;
        hpcat.settings.enable_fabric = false;
    }

    /* Reordered list of ranks ( all ranks on a node before going to the next one) */
    int reordered_ranks[hpcat.num_tasks];
    int pos = 0;
    task.is_first_node_rank = false;
    for (int i = 0; i < hpcat.num_nodes; i++)
    {
        bool is_first_node_rank = true;
        for (int j = 0; j < hpcat.num_tasks; j++)
        {
            if (strncmp(hostnames[i], map[j], HOST_NAME_MAX - 1) == 0)
            {
                if (is_first_node_rank)
                {
                    if (j == task.id)
                        task.is_first_node_rank = true;

                    is_first_node_rank = false;
                }
                reordered_ranks[pos] = j;
                pos++;
            }
        }
    }

    task.is_first_rank = (task.id == reordered_ranks[0]);
    task.is_last_rank = (task.id == reordered_ranks[hpcat.num_tasks - 1]);

    Task *tasks = NULL;
    if (task.is_first_rank)
    {
        tasks = malloc(sizeof(Task) * hpcat.num_tasks);
        if (tasks == NULL)
            FATAL("Error: unable to allocate tasks buffer. Exiting.\n");
    }

    MPI_CHECK( MPI_Gather(&task, sizeof(Task), MPI_BYTE, tasks, sizeof(Task), MPI_BYTE, 0, MPI_COMM_WORLD) );

    if (task.is_first_rank)
    {
        char groups[FABRIC_GROUPS_MAX] = { 0 };

        for (int i = 0; i < hpcat.num_tasks; i++)
        {
            Task *current_task = &tasks[reordered_ranks[i]];

            /* Count total fabric dragonfly groups */
            if (hpcat.settings.enable_fabric && !groups[current_task->fabric_group_id])
            {
                groups[current_task->fabric_group_id] = 1;
                hpcat.num_fabric_groups++;
            }

            /* Count total OpenMP threads */
            if (hpcat.settings.enable_omp)
                hpcat.num_omp_threads += current_task->num_threads;

            hpcat_hint_global_check(&hpcat, current_task);

            /* Print task info */
            switch (hpcat.settings.output_type)
            {
                case STDOUT:
                    hpcat_display_stdout(&hpcat, current_task);
                    break;
                case YAML:
                    hpcat_display_yaml(&hpcat, current_task);
                    break;
            }
        }

        fflush(stdout);
        free(tasks);
    }

    /* Clean up */
    hwloc_topology_destroy(topology);
    MPI_Finalize_noverbose();
    return 0;
}
