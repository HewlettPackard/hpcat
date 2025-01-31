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

#ifndef HPCAT_H
#define HPCAT_H

#include <stdbool.h>
#include <hwloc.h>
#include <mpi.h>
#include "settings.h"

#define STR_MAX      4096
#define NIC_STR_MAX    32

typedef struct
{
    hwloc_bitmap_t  numa_affinity;
    hwloc_cpuset_t  hw_thread_affinity;
    hwloc_cpuset_t  core_affinity;
} Affinity;

typedef struct
{
    int       id;
    Affinity  affinity;
} Thread;

typedef struct
{
    int             num_nic;
    char            name[NIC_STR_MAX];
    hwloc_bitmap_t  numa_affinity;
} Nic;

typedef struct
{
    int             num_accel;
    char            pciaddr[STR_MAX];
    hwloc_bitmap_t  numa_affinity;
    hwloc_bitmap_t  visible_devices;
} Accelerators;

typedef struct
{
    int           id;
    bool          is_first_node_rank;
    bool          is_first_rank;
    bool          is_last_rank;
    Affinity      affinity;
    char          hostname[HOST_NAME_MAX];
    Nic           nic;
    int           num_threads;
    Thread       *threads;
    Accelerators  accel;
} Task;

typedef struct Hpcat
{
    HpcatSettings_t  settings;
    int              num_nodes;
    int              num_tasks;
    Task             task;
    char             mpi_version[MPI_MAX_LIBRARY_VERSION_STRING];
} Hpcat;

#endif /* HPCAT_H */
