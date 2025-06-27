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

#ifndef HPCAT_H
#define HPCAT_H

#include <stdbool.h>
#include <hwloc.h>
#include <mpi.h>
#include "settings.h"

#define STR_MAX              4096
#define NIC_STR_MAX            32
#define BITMAP_ULONGS_MAX       1   /* up to 64 elements */
#define BITMAP_CPU_ULONGS_MAX  32   /* up du 2K hardware threads */
#define THREADS_MAX           (BITMAP_CPU_ULONGS_MAX * 64)

typedef struct
{
    int           num_ulongs;
    unsigned long ulongs[BITMAP_CPU_ULONGS_MAX];
} CPUBitmap;

typedef struct
{
    int           num_ulongs;
    unsigned long ulongs[BITMAP_ULONGS_MAX];
} Bitmap;

typedef struct
{
    Bitmap    numa_affinity;
    CPUBitmap hw_thread_affinity;
    CPUBitmap core_affinity;
} Affinity;

typedef struct
{
    int      id;
    Affinity affinity;
} Thread;

typedef struct
{
    int  num_nic;
    char name[NIC_STR_MAX];
    char numa_affinity;
} Nic;

typedef struct
{
    int    num_accel;
    char   pciaddr[STR_MAX];
    Bitmap numa_affinity;
    Bitmap visible_devices;
} Accelerators;

typedef struct
{
    int           id;
    bool          is_first_node_rank;
    bool          is_first_rank;
    bool          is_last_rank;
    bool          is_mpich_ofi_nic_policy_gpu;
    Affinity      affinity;
    char          hostname[HOST_NAME_MAX];
    int           fabric_group_id;
    Nic           nic;
    int           num_threads;
    Thread        threads[THREADS_MAX];
    Accelerators  accel;
} Task;

typedef struct Hpcat
{
    HpcatSettings_t  settings;
    int              num_fabric_groups;
    int              num_nodes;
    int              num_tasks;
    int              num_omp_threads;
    int              id;
    char             mpi_version[MPI_MAX_LIBRARY_VERSION_STRING];
} Hpcat;

#endif /* HPCAT_H */
