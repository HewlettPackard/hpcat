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
* output.c: Controlling the output format.
*
* URL       https://github.com/HewlettPackard/hpcat
******************************************************************************/

#include <unistd.h>
#include <string.h>
#include <stdio.h>
#include <limits.h>
#include <hwloc.h>
#include <stdlib.h>

#include "output.h"
#include "settings.h"

#define QUOTE(name) #name
#define STR(macro) QUOTE(macro)

#define STR_MAX      4096

/* Bytes required to encode ext ASCII char */
#define EXT_SZ 3

/* Column width (not accounting for margins) */
#define HOST_W    15
#define MPI_W      4
#define NIC_W     20
#define NIC_W1    10
#define ACCEL_W   43
#define ACCEL_W1   7
#define ACCEL_W2  23
#define OMP_W      4
#define CPU_W     46
#define CPU_W1    20
#define CPU_W2    13
#define NUMA_W     7

#define MARGINS_W  2
#define SPLIT_W    1

#define H_SPLIT  "═══════════════════════════════════════════════════════════════════════════════════════════════════════════════════════════════════════════"
#define H_DASH   "-------------------------------------------------------------------------------------------------------------------------------------------"
#define H_SPACE  "                                                                                                                                           "
#define TITLE    "HPC Affinity Tracker"
#define VERSION  "v" HPCAT_VERSION " "

static void stdout_header(Hpcat *handle)
{
    HpcatSettings_t *settings = &handle->settings;
    char line[3][STR_MAX] = { '\0' };
    int ret = { 0 };
    int all_col = (HOST_W + MPI_W + CPU_W + 3 * MARGINS_W + 2 * SPLIT_W);

    ret += sprintf(line[2] + ret, "%s%.*s╦%.*s╦",  settings->enable_banner ? "╠" : "╔",
                   (HOST_W + MARGINS_W) * EXT_SZ, H_SPLIT, (MPI_W + MARGINS_W) * EXT_SZ, H_SPLIT);

    if (settings->enable_nic)
    {
        all_col += NIC_W + MARGINS_W + SPLIT_W;
        ret += sprintf(line[2] + ret, "%.*s╦", (NIC_W + MARGINS_W) * EXT_SZ, H_SPLIT);
    }

    if (settings->enable_accel)
    {
        all_col += ACCEL_W + MARGINS_W + SPLIT_W;
        ret += sprintf(line[2] + ret, "%.*s╦", (ACCEL_W + MARGINS_W) * EXT_SZ, H_SPLIT);
    }

    if (settings->enable_omp)
    {
        all_col += OMP_W + MARGINS_W + SPLIT_W;
        ret += sprintf(line[2] + ret, "%.*s╦", (OMP_W + MARGINS_W) * EXT_SZ, H_SPLIT);
    }

    sprintf(line[2] + ret, "%.*s%s", (CPU_W + MARGINS_W) * EXT_SZ, H_SPLIT, settings->enable_banner ? "╣" : "╗");
    if (!settings->enable_banner)
    {
        printf("%s\n", line[2]);
        return;
    }

    sprintf(line[0], "╔%.*s╗", all_col * EXT_SZ, H_SPLIT);

    /* Center title */
    const int s_left = (all_col - (int)strlen(TITLE)) / 2;
    const int s_right = all_col - (s_left + (int)strlen(TITLE) + (int)strlen(VERSION));
    sprintf(line[1], "║%.*s%s%.*s%s║", s_left, H_SPACE, TITLE, s_right, H_SPACE, VERSION);

    printf("%s\n%s\n%s\n%s\n", handle->mpi_version, line[0], line[1], line[2]);
}

static void stdout_titles(Hpcat *handle)
{
    HpcatSettings_t *settings = &handle->settings;
    char line[2][STR_MAX] = { '\0' };
    int ret[2] = { 0 };

    ret[0] += sprintf(line[0] + ret[0], "║ %" STR(HOST_W) "s ║ %" STR(MPI_W) "s ║", "HOST", "MPI");
    ret[1] += sprintf(line[1] + ret[1], "║ %" STR(HOST_W) "s ║ %" STR(MPI_W) "s ║", "(NODE)", "RANK");

    if (settings->enable_nic)
    {
        ret[0] += sprintf(line[0] + ret[0], " %" STR(NIC_W) "s ║", "NETWORK     ");
        ret[1] += sprintf(line[1] + ret[1], " %" STR(NIC_W1) "s │ %" STR(NUMA_W) "s ║", "INTERFACE", "NUMA");
    }

    if (settings->enable_accel)
    {
        ret[0] += sprintf(line[0] + ret[0], " %" STR(ACCEL_W) "s ║", "ACCELERATORS               ");
        ret[1] += sprintf(line[1] + ret[1], " %" STR(ACCEL_W1) "s │ %" STR(ACCEL_W2) "s │ %" STR(NUMA_W) "s ║", "ID", "PCIE ADDR.", "NUMA");
    }

    if (settings->enable_omp)
    {
        ret[0] += sprintf(line[0] + ret[0], " %" STR(OMP_W) "s ║", "OMP");
        ret[1] += sprintf(line[1] + ret[1], " %" STR(OMP_W) "s ║", "ID");
    }

    sprintf(line[0] + ret[0], " %" STR(CPU_W) "s ║", "CPU                    ");
    sprintf(line[1] + ret[1], " %" STR(CPU_W1) "s │ %" STR(CPU_W2) "s │ %" STR(NUMA_W) "s ║", "LOGICAL PROC", "PHYSICAL CORE", "NUMA");
    printf("%s\n%s\n", line[0], line[1]);
}

static void stdout_split_middle(Hpcat *handle, const bool high)
{
    HpcatSettings_t *settings = &handle->settings;
    char line[STR_MAX] = { '\0' };
    int ret = { 0 };

    ret += sprintf(line + ret, "╠%.*s╬%.*s╬", (HOST_W + MARGINS_W) * EXT_SZ, H_SPLIT,
                                               (MPI_W + MARGINS_W) * EXT_SZ, H_SPLIT);

    if (settings->enable_nic)
        ret += sprintf(line + ret, "%.*s%s%.*s%s", (NIC_W1 + MARGINS_W) * EXT_SZ, H_SPLIT, high ? "╪" : "╧",
                                                   (NUMA_W + MARGINS_W) * EXT_SZ, H_SPLIT, "╬");

    if (settings->enable_accel)
        ret += sprintf(line + ret, "%.*s%s%.*s%s%.*s%s", (ACCEL_W1 + MARGINS_W) * EXT_SZ, H_SPLIT, high ? "╪" : "╧",
                                                         (ACCEL_W2 + MARGINS_W) * EXT_SZ, H_SPLIT, high ? "╪" : "╧",
                                                           (NUMA_W + MARGINS_W) * EXT_SZ, H_SPLIT, "╬");

    if (settings->enable_omp)
        ret += sprintf(line + ret, "%.*s%s", (OMP_W + MARGINS_W) * EXT_SZ, H_SPLIT, "╬");

    sprintf(line + ret, "%.*s%s%.*s%s%.*s%s", (CPU_W1 + MARGINS_W) * EXT_SZ, H_SPLIT, high ? "╪" : "╧",
                                              (CPU_W2 + MARGINS_W) * EXT_SZ, H_SPLIT, high ? "╪" : "╧",
                                              (NUMA_W + MARGINS_W) * EXT_SZ, H_SPLIT, "╣");
    printf("%s\n", line);
}

static void stdout_split_end(Hpcat *handle)
{
    HpcatSettings_t *settings = &handle->settings;
    char line[STR_MAX] = { '\0' };
    int ret = { 0 };

    if (settings->enable_banner)
        ret += sprintf(line + ret, "╠%.*s╬%.*s╬", (HOST_W + MARGINS_W) * EXT_SZ, H_SPLIT,
                                                   (MPI_W + MARGINS_W) * EXT_SZ, H_SPLIT);
    else
        ret += sprintf(line + ret, "╚%.*s╩%.*s╩", (HOST_W + MARGINS_W) * EXT_SZ, H_SPLIT,
                                                   (MPI_W + MARGINS_W) * EXT_SZ, H_SPLIT);

    if (settings->enable_nic)
        ret += sprintf(line + ret, "%.*s%s%.*s%s", (NIC_W1 + MARGINS_W) * EXT_SZ, H_SPLIT, "╧",
                                                   (NUMA_W + MARGINS_W) * EXT_SZ, H_SPLIT, "╩");

    if (settings->enable_accel)
        ret += sprintf(line + ret, "%.*s%s%.*s%s%.*s%s", (ACCEL_W1 + MARGINS_W) * EXT_SZ, H_SPLIT, "╧",
                                                         (ACCEL_W2 + MARGINS_W) * EXT_SZ, H_SPLIT, "╧",
                                                           (NUMA_W + MARGINS_W) * EXT_SZ, H_SPLIT, "╩");

    if (settings->enable_omp)
        ret += sprintf(line + ret, "%.*s%s", (OMP_W + MARGINS_W) * EXT_SZ, H_SPLIT, "╩");

    sprintf(line + ret, "%.*s%s%.*s%s%.*s%s", (CPU_W1 + MARGINS_W) * EXT_SZ, H_SPLIT, "╧",
                                              (CPU_W2 + MARGINS_W) * EXT_SZ, H_SPLIT, "╧",
                                              (NUMA_W + MARGINS_W) * EXT_SZ, H_SPLIT, "╝");
    printf("%s\n", line);
}

static void stdout_node(Hpcat *handle)
{
    HpcatSettings_t *settings = &handle->settings;
    Task *task = &handle->task;
    char line[STR_MAX] = { '\0' };
    int ret = { 0 };

    ret += sprintf(line + ret, "║ %" STR(HOST_W) "s ║ %" STR(MPI_W) "s ║", task->hostname, " ");

    if (settings->enable_nic)
        ret += sprintf(line + ret, " %" STR(NIC_W1) "s │ %" STR(NUMA_W) "s ║", " ", " ");

    if (settings->enable_accel)
        ret += sprintf(line + ret, " %" STR(ACCEL_W1) "s │ %" STR(ACCEL_W2) "s │ %" STR(NUMA_W) "s ║", " ", " ", " ");

    if (settings->enable_omp)
        ret += sprintf(line + ret, " %" STR(OMP_W) "s ║", " ");

    sprintf(line + ret, " %" STR(CPU_W1) "s │ %" STR(CPU_W2) "s │ %" STR(NUMA_W) "s ║", " ", " ", " ");
    printf("%s\n", line);
}

static void stdout_task(Hpcat *handle)
{
    HpcatSettings_t *settings = &handle->settings;
    Task *task = &handle->task;
    char line[STR_MAX] = { '\0' };
    int ret = { 0 };

    char hw_thread_str[STR_MAX], core_str[STR_MAX], numa_str[STR_MAX];
    char nic_numa_str[STR_MAX] = { 0 }, accel_numa_str[STR_MAX] = { 0 }, accel_visible_str[STR_MAX] = { 0 };
    hwloc_bitmap_list_snprintf(hw_thread_str, STR_MAX - 1, task->affinity.hw_thread_affinity);
    hwloc_bitmap_list_snprintf(core_str, STR_MAX - 1, task->affinity.core_affinity);
    hwloc_bitmap_list_snprintf(numa_str, STR_MAX - 1, task->affinity.numa_affinity);

    if (task->accel.num_accel > 0)
    {
        hwloc_bitmap_list_snprintf(accel_numa_str, STR_MAX - 1, task->accel.numa_affinity);
        hwloc_bitmap_list_snprintf(accel_visible_str, STR_MAX - 1, task->accel.visible_devices);
    }

    if (task->nic.num_nic > 0)
        hwloc_bitmap_list_snprintf(nic_numa_str, STR_MAX - 1, task->nic.numa_affinity);

    ret += sprintf(line + ret, "║ %" STR(HOST_W) "s ║ %" STR(MPI_W) "d ║", " ", task->id);

    if (settings->enable_nic)
        ret += sprintf(line + ret, " %" STR(NIC_W1) "s │ %" STR(NUMA_W) "s ║", task->nic.name, nic_numa_str);

    if (settings->enable_accel)
        ret += sprintf(line + ret, " %" STR(ACCEL_W1) "s │ %" STR(ACCEL_W2) "s │ %" STR(NUMA_W) "s ║",
                       accel_visible_str, task->accel.pciaddr, accel_numa_str);

    if (settings->enable_omp)
        ret += sprintf(line + ret, " %.*s ║", OMP_W, H_DASH);

    sprintf(line + ret, " %" STR(CPU_W1) "s │ %" STR(CPU_W2) "s │ %" STR(NUMA_W) "s ║", hw_thread_str, core_str, numa_str);
    printf("%s\n", line);
}

static void stdout_omp(Hpcat *handle)
{
    HpcatSettings_t *settings = &handle->settings;
    Task *task = &handle->task;
    char line[STR_MAX] = { '\0' };
    int ret = { 0 };

    char hw_thread_str[STR_MAX], core_str[STR_MAX], numa_str[STR_MAX];

    if (!settings->enable_omp)
        return;

    ret += sprintf(line + ret, "║ %" STR(HOST_W) "s ║ %" STR(MPI_W) "s ║", " ", " ");

    if (settings->enable_nic)
        ret += sprintf(line + ret, " %" STR(NIC_W1) "s │ %" STR(NUMA_W) "s ║", " ", " ");

    if (settings->enable_accel)
        ret += sprintf(line + ret, " %" STR(ACCEL_W1) "s │ %" STR(ACCEL_W2) "s │ %" STR(NUMA_W) "s ║", " ", " ", " ");

    for (int i = 0; i < task->num_threads; i++)
    {
        Thread *thread = &task->threads[i];
        hwloc_bitmap_list_snprintf(hw_thread_str, STR_MAX - 1, thread->affinity.hw_thread_affinity);
        hwloc_bitmap_list_snprintf(core_str, STR_MAX - 1, thread->affinity.core_affinity);
        hwloc_bitmap_list_snprintf(numa_str, STR_MAX - 1, thread->affinity.numa_affinity);
        printf("%s %" STR(OMP_W) "d ║ %" STR(CPU_W1) "s │ %" STR(CPU_W2) "s │ %" STR(NUMA_W) "s ║\n", line, thread->id, hw_thread_str, core_str, numa_str);
    }
}

static void stdout_footer(Hpcat *handle)
{
    printf("║ %.*s%s%4d ║ %" STR(MPI_W) "d ║\n" , HOST_W - 11, H_SPACE , "TOTAL: ", handle->num_nodes, handle->num_tasks);
    printf("╚%.*s╩%.*s╝\n", (HOST_W + MARGINS_W) * EXT_SZ, H_SPLIT, (MPI_W + MARGINS_W) * EXT_SZ, H_SPLIT);
}

/**
 * Output data in human readble format (stdout)
 *
 * @param   handle[in]          Hpcat handle
 */
void hpcat_display_stdout(Hpcat *handle)
{
    Task *task = &handle->task;

    /* First (reordered) rank prints the header */
    if (task->is_first_rank)
    {
        stdout_header(handle);
        stdout_titles(handle);
    }

    /* Node level */
    if (task->is_first_node_rank)
    {
        stdout_split_middle(handle, true);
        stdout_node(handle);
    }

    /* Task level */
    stdout_task(handle);

    /* OMP thread level */
    stdout_omp(handle);

    if (task->is_last_rank)
    {
        if (handle->settings.enable_banner)
        {
            stdout_split_middle(handle, false);
            stdout_titles(handle);
            stdout_split_end(handle);
            stdout_footer(handle);
        }
        else
            stdout_split_end(handle);
    }

    fflush(stdout);
}

/**
 * Output data in yaml format (stdout)
 *
 * @param   handle[in]          Hpcat handle
 */
void hpcat_display_yaml(Hpcat *handle)
{
    Task *task = &handle->task;
    char hw_thread_str[STR_MAX], core_str[STR_MAX], numa_str[STR_MAX];

    hwloc_bitmap_list_snprintf(hw_thread_str, STR_MAX - 1, task->affinity.hw_thread_affinity);
    hwloc_bitmap_list_snprintf(core_str, STR_MAX - 1, task->affinity.core_affinity);
    hwloc_bitmap_list_snprintf(numa_str, STR_MAX - 1, task->affinity.numa_affinity);

    if (task->is_first_rank)
    {
        printf("mpiversion: \"%s\"\n", handle->mpi_version);
        printf("nodes:\n");
    }

    /* Node level */
    if (task->is_first_node_rank)
    {
        printf("%2s- name: \"%s\"\n", " ", task->hostname);
        printf("%4smpi:\n", " ");
    }

    /* Task level */
    printf("%6s- rank: %d\n", " ", task->id);
    printf("%8slogical_proc: \"%s\"\n", " ", hw_thread_str);
    printf("%8sphysical_core: \"%s\"\n", " ", core_str);
    printf("%8snuma: \"%s\"\n", " ", numa_str);

    if (task->nic.num_nic > 0)
    {
        char nic_numa_str[STR_MAX] = { 0 };
        hwloc_bitmap_list_snprintf(nic_numa_str, STR_MAX - 1, task->nic.numa_affinity);
        printf("%8snetwork:\n", " ");
        printf("%10s- interface: \"%s\"\n", " ", task->nic.name);
        printf("%12snuma: \"%s\"\n", " ", nic_numa_str);
    }

    if (task->accel.num_accel > 0)
    {
        char accel_numa_str[STR_MAX] = { 0 };
        char accel_visible_str[STR_MAX] = { 0 };
        hwloc_bitmap_list_snprintf(accel_numa_str, STR_MAX - 1, task->accel.numa_affinity);
        hwloc_bitmap_list_snprintf(accel_visible_str, STR_MAX - 1, task->accel.visible_devices);
        printf("%8saccelerators:\n", " ");
        printf("%10s- visible: \"%s\"\n", " ", accel_visible_str);
        printf("%10spci: \"%s\"\n", " ", task->accel.pciaddr);
        printf("%12snuma: \"%s\"\n", " ", accel_numa_str);
    }

    /* OMP thread level */
    if (task->num_threads > 1 && handle->settings.enable_omp)
    {
        printf("%8somp:\n", " ");
        for (int i = 0; i < task->num_threads; i++)
        {
            Thread *thread = &task->threads[i];
            hwloc_bitmap_list_snprintf(hw_thread_str, STR_MAX - 1, thread->affinity.hw_thread_affinity);
            hwloc_bitmap_list_snprintf(core_str, STR_MAX - 1, thread->affinity.core_affinity);
            hwloc_bitmap_list_snprintf(numa_str, STR_MAX - 1, thread->affinity.numa_affinity);

            printf("%10s- thread: %d\n", " ", thread->id);
            printf("%12slogical_proc: \"%s\"\n", " ", hw_thread_str);
            printf("%12sphysical_core: \"%s\"\n", " ", core_str);
            printf("%12snuma: \"%s\"\n", " ", numa_str);
        }
    }
    fflush(stdout);
}
