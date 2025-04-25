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
* output.c: Controlling the output format.
*
* URL       https://github.com/HewlettPackard/hpcat
******************************************************************************/

#include <unistd.h>
#include <string.h>
#include <stdio.h>
#include <limits.h>
#include <stdlib.h>

#include "hwloc.h"
#include "fort.h"
#include "output.h"
#include "common.h"
#include "settings.h"

#define STR_MAX      4096
#define INT_STR_MAX    10

#define TITLE    "HPC Affinity Tracker"
#define VERSION  "v" HPCAT_VERSION

#define HOST_COL   1
#define MPI_COL    1
#define OMP_COL    1
#define CPU_COL    3
#define ACCEL_COL  3
#define NIC_COL    2
#define FABRIC_COL 1

ft_table_t *table = NULL;
size_t num_columns = 0;
size_t num_rows = 0;
size_t start_host, start_mpi, start_omp, start_cpu, start_accel, start_nic;


static void stdout_header(Hpcat *handle)
{
    HpcatSettings_t *settings = &handle->settings;

    printf("%s\n", handle->mpi_version);

    /* Configuring the header */
    if (settings->enable_color)
    {
        ft_set_cell_prop(table, 0, FT_ANY_COLUMN, FT_CPROP_CONT_FG_COLOR, FT_COLOR_BLACK);
        ft_set_cell_prop(table, 0, FT_ANY_COLUMN, FT_CPROP_CELL_BG_COLOR, FT_COLOR_YELLOW);
        ft_set_cell_prop(table, 0, FT_ANY_COLUMN, FT_CPROP_CONT_TEXT_STYLE, FT_TSTYLE_BOLD);
    }

    ft_set_cell_prop(table, 0, FT_ANY_COLUMN, FT_CPROP_ROW_TYPE, FT_ROW_HEADER);
    ft_set_cell_prop(table, 0, FT_ANY_COLUMN, FT_CPROP_TEXT_ALIGN, FT_ALIGNED_CENTER);
    ft_set_cell_span(table, 0, 0, num_columns);
    ft_write_ln(table, TITLE " (" VERSION ")");

    num_rows ++;
}

static void stdout_footer(Hpcat *handle)
{
    char omp_str[INT_STR_MAX], fabric_str[INT_STR_MAX], row_str[STR_MAX];

    HpcatSettings_t *settings = &handle->settings;

    if (settings->enable_omp)
        sprintf(omp_str, "%d", handle->num_omp_threads);

    if (settings->enable_fabric)
        sprintf(fabric_str, "%d|", handle->num_fabric_groups);

    sprintf(row_str, "TOTAL: %s%d|%d|%s", (settings->enable_fabric ? fabric_str : ""),
                                          handle->num_nodes, handle->num_tasks,
                                          (settings->enable_omp ? omp_str : ""));

    ft_printf_ln(table, row_str);

    if (settings->enable_color)
        ft_set_cell_prop(table, num_rows, FT_ANY_COLUMN, FT_CPROP_CONT_FG_COLOR, FT_COLOR_CYAN);

    ft_set_cell_span(table, num_rows, start_cpu, num_columns - start_cpu);
    ft_set_cell_prop(table, num_rows, start_cpu, FT_CPROP_TEXT_ALIGN, FT_ALIGNED_CENTER);
}

static void stdout_titles(Hpcat *handle)
{
    HpcatSettings_t *settings = &handle->settings;
    char row_str[STR_MAX];

    /* First title row */
    sprintf(row_str, "%sHOST|MPI|%sCPU||%s%s", (settings->enable_fabric ? "FABRIC|" : "" ),
                                               (settings->enable_omp ? "OMP|" : "" ),
                                               (settings->enable_accel ? "|ACCELERATORS||" : ""),
                                               (settings->enable_nic ? "|NETWORK|" : ""));
    ft_printf_ln(table, row_str);

    if (settings->enable_color)
    {
        ft_set_cell_prop(table, num_rows, FT_ANY_COLUMN, FT_CPROP_CONT_TEXT_STYLE, FT_TSTYLE_BOLD);
        ft_set_cell_prop(table, num_rows, FT_ANY_COLUMN, FT_CPROP_CONT_FG_COLOR, FT_COLOR_CYAN);
    }

    ft_set_cell_prop(table, num_rows, FT_ANY_COLUMN, FT_CPROP_TEXT_ALIGN, FT_ALIGNED_CENTER);
    ft_set_cell_prop(table, num_rows, start_host, FT_CPROP_TEXT_ALIGN, FT_ALIGNED_RIGHT);
    ft_set_cell_prop(table, num_rows, start_mpi, FT_CPROP_TEXT_ALIGN, FT_ALIGNED_RIGHT);

    ft_set_cell_span(table, num_rows, start_cpu, 3);

    if (settings->enable_omp)
        ft_set_cell_prop(table, num_rows, start_omp, FT_CPROP_TEXT_ALIGN, FT_ALIGNED_RIGHT);
    if (settings->enable_accel)
        ft_set_cell_span(table, num_rows, start_accel, 3);
    if (settings->enable_nic)
        ft_set_cell_span(table, num_rows, start_nic, 2);

    num_rows++;

    /* Second title row */
    sprintf(row_str, "%s(NODE)|RANK|%sLOGICAL PROC|PHYSICAL CORE|NUMA%s%s",
                                         (settings->enable_fabric ? "GROUP ID|" : "" ),
                                         (settings->enable_omp ? "ID|" : "" ),
                                         (settings->enable_accel ? "|ID|PCIE ADDR.|NUMA" : ""),
                                         (settings->enable_nic ? "|INTERFACE|NUMA" : ""));

    ft_printf_ln(table, row_str);

    if (settings->enable_color)
    {
        ft_set_cell_prop(table, num_rows, FT_ANY_COLUMN, FT_CPROP_CONT_TEXT_STYLE, FT_TSTYLE_ITALIC);
        ft_set_cell_prop(table, num_rows, FT_ANY_COLUMN, FT_CPROP_CONT_FG_COLOR, FT_COLOR_CYAN);
        ft_set_cell_prop(table, num_rows, start_host, FT_CPROP_CONT_TEXT_STYLE, FT_TSTYLE_BOLD);
        ft_set_cell_prop(table, num_rows, start_mpi, FT_CPROP_CONT_TEXT_STYLE, FT_TSTYLE_BOLD);

        if (settings->enable_omp)
            ft_set_cell_prop(table, num_rows, start_omp, FT_CPROP_CONT_TEXT_STYLE, FT_TSTYLE_BOLD);

        if (settings->enable_accel)
            ft_set_cell_prop(table, num_rows, start_accel, FT_CPROP_CONT_TEXT_STYLE, FT_TSTYLE_ITALIC);
    }

    ft_set_cell_prop(table, num_rows, FT_ANY_COLUMN, FT_CPROP_TEXT_ALIGN, FT_ALIGNED_CENTER);
    ft_set_cell_prop(table, num_rows, start_host, FT_CPROP_TEXT_ALIGN, FT_ALIGNED_RIGHT);
    ft_set_cell_prop(table, num_rows, start_mpi, FT_CPROP_TEXT_ALIGN, FT_ALIGNED_RIGHT);

    if (settings->enable_omp)
        ft_set_cell_prop(table, num_rows, start_omp, FT_CPROP_TEXT_ALIGN, FT_ALIGNED_RIGHT);

    num_rows++;
}

static void bitmap_to_str(char *str, Bitmap *bitmap, hwloc_bitmap_t tmp)
{
    hwloc_bitmap_zero(tmp);
    hwloc_bitmap_from_ulongs(tmp, bitmap->num_ulongs, bitmap->ulongs);
    hwloc_bitmap_list_snprintf(str, STR_MAX - 1, tmp);
}

static void stdout_task(Hpcat *handle, Task *task)
{
    HpcatSettings_t *settings = &handle->settings;
    char hw_thread_str[STR_MAX], core_str[STR_MAX], numa_str[STR_MAX], row_str[STR_MAX];
    char nic_numa_str[STR_MAX] = { 0 }, accel_numa_str[STR_MAX] = { 0 }, accel_visible_str[STR_MAX] = { 0 };

    hwloc_bitmap_t bitmap = hwloc_bitmap_alloc();
    if (bitmap == NULL)
        FATAL("Error: Unable to allocate temporary bitmap. Exiting.\n");

    bitmap_to_str(hw_thread_str, (Bitmap*)&task->affinity.hw_thread_affinity, bitmap);
    bitmap_to_str(core_str, (Bitmap*)&task->affinity.core_affinity, bitmap);
    bitmap_to_str(numa_str, &task->affinity.numa_affinity, bitmap);

    if (task->accel.num_accel > 0)
    {
        bitmap_to_str(accel_numa_str, &task->accel.numa_affinity, bitmap);
        bitmap_to_str(accel_visible_str, &task->accel.visible_devices, bitmap);
    }

    hwloc_bitmap_free(bitmap);

    if (task->nic.num_nic > 0)
        sprintf(nic_numa_str, "%d", task->nic.numa_affinity);

    sprintf(row_str, "%s|%d|%s%s|%s|%s%s%s%s%s%s%s%s%s%s%s",
                                     (settings->enable_fabric ? "|" : ""),
                                     task->id,
                                     (settings->enable_omp ? "---|" : "" ),
                                     hw_thread_str, core_str, numa_str,
                                     (settings->enable_accel ? "|" : ""),
                                     (settings->enable_accel ? accel_visible_str : ""),
                                     (settings->enable_accel ? "|" : ""),
                                     (settings->enable_accel ? task->accel.pciaddr : ""),
                                     (settings->enable_accel ? "|" : ""),
                                     (settings->enable_accel ? accel_numa_str : ""),
                                     (settings->enable_nic ? "|" : ""),
                                     (settings->enable_nic ? task->nic.name : ""),
                                     (settings->enable_nic ? "|" : ""),
                                     (settings->enable_nic ? nic_numa_str : ""));
    ft_printf_ln(table, row_str);

    if (settings->enable_color)
        ft_set_cell_prop(table, num_rows, FT_ANY_COLUMN, FT_CPROP_CONT_FG_COLOR, FT_COLOR_LIGHT_GRAY);

    num_rows++;
}

static void stdout_omp(Hpcat *handle, Task *task)
{
    HpcatSettings_t *settings = &handle->settings;
    char hw_thread_str[STR_MAX], core_str[STR_MAX], numa_str[STR_MAX], row_str[STR_MAX];

    hwloc_bitmap_t bitmap = hwloc_bitmap_alloc();
    if (bitmap == NULL)
        FATAL("Error: Unable to allocate temporary bitmap. Exiting.\n");

    for (int i = 0; i < task->num_threads; i++)
    {
        Thread *thread = &task->threads[i];

        bitmap_to_str(hw_thread_str, (Bitmap*)&thread->affinity.hw_thread_affinity, bitmap);
        bitmap_to_str(core_str, (Bitmap*)&thread->affinity.core_affinity, bitmap);
        bitmap_to_str(numa_str, &thread->affinity.numa_affinity, bitmap);

        sprintf(row_str, "%s||%d|%s|%s|%s", (settings->enable_fabric ? "|" : ""),
                                            thread->id, hw_thread_str, core_str, numa_str);

        ft_printf_ln(table, row_str);
        num_rows++;
    }

    hwloc_bitmap_free(bitmap);
}

/**
 * Output data in human readble format (stdout)
 *
 * @param   handle[in]          Hpcat handle
 */
void hpcat_display_stdout(Hpcat *handle, Task *task)
{
    HpcatSettings_t *settings = &handle->settings;

    /* First (reordered) rank prints the header */
    if (task->is_first_rank)
    {
        /* Initialize the table */
        table = ft_create_table();
        ft_set_border_style(table, FT_SOLID_ROUND_STYLE);
        ft_set_cell_prop(table, FT_ANY_ROW, FT_ANY_COLUMN, FT_CPROP_CONT_FG_COLOR, FT_COLOR_YELLOW);
        ft_set_cell_prop(table, FT_ANY_ROW, FT_ANY_COLUMN, FT_CPROP_TEXT_ALIGN, FT_ALIGNED_RIGHT);

        /* Compute amount of columns */
        num_columns = HOST_COL + MPI_COL + CPU_COL;
        if (settings->enable_fabric)
            num_columns += FABRIC_COL;
        if (settings->enable_omp)
            num_columns += OMP_COL;
        if (settings->enable_accel)
            num_columns += ACCEL_COL;
        if (settings->enable_nic)
            num_columns += NIC_COL;

        /* Compute start column of each section */
        start_host = (settings->enable_fabric ? FABRIC_COL : 0);
        start_mpi = start_host + HOST_COL;
        start_omp = start_mpi + MPI_COL;
        start_cpu = start_omp + (settings->enable_omp ? OMP_COL : 0);
        start_accel = start_cpu + CPU_COL;
        start_nic = start_accel + (settings->enable_accel ? ACCEL_COL : 0);

        if (settings->enable_color)
        {
            ft_set_cell_prop(table, FT_ANY_ROW, 0, FT_CPROP_CONT_TEXT_STYLE, FT_TSTYLE_BOLD);
            ft_set_cell_prop(table, FT_ANY_ROW, start_host, FT_CPROP_CONT_TEXT_STYLE, FT_TSTYLE_BOLD);
            ft_set_cell_prop(table, FT_ANY_ROW, start_mpi, FT_CPROP_CONT_TEXT_STYLE, FT_TSTYLE_BOLD);

            if (settings->enable_omp)
                ft_set_cell_prop(table, FT_ANY_ROW, start_omp, FT_CPROP_CONT_TEXT_STYLE, FT_TSTYLE_BOLD);

            if (settings->enable_accel)
                ft_set_cell_prop(table, FT_ANY_ROW, start_accel, FT_CPROP_CONT_TEXT_STYLE, FT_TSTYLE_BOLD);
        }

        if (settings->enable_banner)
            stdout_header(handle);

        stdout_titles(handle);
    }

    /* Node level */
    if (task->is_first_node_rank)
    {
        char row_str[STR_MAX];
        ft_add_separator(table);

        if (settings->enable_fabric)
            sprintf(row_str, "%d|%s|", task->fabric_group_id, task->hostname);
        else
            sprintf(row_str, "%s|", task->hostname);

        ft_printf_ln(table, row_str);

        if (settings->enable_color)
            ft_set_cell_prop(table, num_rows, FT_ANY_COLUMN, FT_CPROP_CONT_FG_COLOR, FT_COLOR_LIGHT_GRAY);

        num_rows++;
    }

    /* Task level */
    stdout_task(handle, task);

    /* OMP thread level */
    if (settings->enable_omp)
        stdout_omp(handle, task);

    if (task->is_last_rank)
    {
        if (settings->enable_banner)
        {
            ft_add_separator(table);
            stdout_titles(handle);
            ft_add_separator(table);
            stdout_footer(handle);
        }

        /* Dump the table */
        printf("%s\n", ft_to_string(table));
        ft_destroy_table(table);
    }
}

/**
 * Output data in yaml format (stdout)
 *
 * @param   handle[in]          Hpcat handle
 */
void hpcat_display_yaml(Hpcat *handle, Task *task)
{
    HpcatSettings_t *settings = &handle->settings;
    char hw_thread_str[STR_MAX], core_str[STR_MAX], numa_str[STR_MAX];

    hwloc_bitmap_t bitmap = hwloc_bitmap_alloc();
    if (bitmap == NULL)
        FATAL("Error: Unable to allocate temporary bitmap. Exiting.\n");

    bitmap_to_str(hw_thread_str, (Bitmap*)&task->affinity.hw_thread_affinity, bitmap);
    bitmap_to_str(core_str, (Bitmap*)&task->affinity.core_affinity, bitmap);
    bitmap_to_str(numa_str, &task->affinity.numa_affinity, bitmap);

    if (task->is_first_rank)
    {
        printf("mpiversion: \"%s\"\n", handle->mpi_version);
        printf("nodes:\n");
    }

    /* Node level */
    if (task->is_first_node_rank)
    {
        printf("%2s- name: \"%s\"\n", " ", task->hostname);

        if (settings->enable_fabric)
            printf("%4sfabric_group_id: %d\n", " ", task->fabric_group_id);

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
        sprintf(nic_numa_str, "%d", task->nic.numa_affinity);
        printf("%8snetwork:\n", " ");
        printf("%10s- interface: \"%s\"\n", " ", task->nic.name);
        printf("%12snuma: \"%s\"\n", " ", nic_numa_str);
    }

    if (task->accel.num_accel > 0)
    {
        char accel_numa_str[STR_MAX] = { 0 };
        char accel_visible_str[STR_MAX] = { 0 };
        bitmap_to_str(accel_numa_str, &task->accel.numa_affinity, bitmap);
        bitmap_to_str(accel_visible_str, &task->accel.visible_devices, bitmap);
        printf("%8saccelerators:\n", " ");
        printf("%10s- visible: \"%s\"\n", " ", accel_visible_str);
        printf("%12spci: \"%s\"\n", " ", task->accel.pciaddr);
        printf("%12snuma: \"%s\"\n", " ", accel_numa_str);
    }

    /* OMP thread level */
    if (task->num_threads > 1 && settings->enable_omp)
    {
        printf("%8somp:\n", " ");
        for (int i = 0; i < task->num_threads; i++)
        {
            Thread *thread = &task->threads[i];
            bitmap_to_str(hw_thread_str, (Bitmap*)&task->affinity.hw_thread_affinity, bitmap);
            bitmap_to_str(core_str, (Bitmap*)&task->affinity.core_affinity, bitmap);
            bitmap_to_str(numa_str, &task->affinity.numa_affinity, bitmap);

            printf("%10s- thread: %d\n", " ", thread->id);
            printf("%12slogical_proc: \"%s\"\n", " ", hw_thread_str);
            printf("%12sphysical_core: \"%s\"\n", " ", core_str);
            printf("%12snuma: \"%s\"\n", " ", numa_str);
        }
    }

    if (task->is_last_rank)
    {
        if (settings->enable_fabric)
            printf("total_fabric_groups: %d\n", handle->num_fabric_groups);

        printf("total_nodes: %d\n", handle->num_nodes);
        printf("total_mpi_ranks: %d\n", handle->num_tasks);

        if (settings->enable_omp)
            printf("total_omp_threads: %d\n", handle->num_omp_threads);
    }

    hwloc_bitmap_free(bitmap);
}
