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
* hints.h: Hint detection and management
*
* URL       https://github.com/HewlettPackard/hpcat
******************************************************************************/

#ifndef HPCAT_HINT_H
#define HPCAT_HINT_H

#include "hpcat.h"

typedef enum HintType
{
    HINT_SHARED_CORES = 0,        /* Indicates shared CPU cores between tasks */
    HINT_MULTIPLE_NUMA_NODES,     /* Multiple NUMA nodes used by a task       */
    HINT_DIFFERENT_CPU_GPU_NUMA,  /* Different NUMA for CPU and GPU           */
    HINT_DIFFERENT_CPU_NIC_NUMA,  /* Different NUMA for CPU and NIC           */
    HINT_DIFFERENT_GPU_NIC_NUMA,  /* Different NUMA for GPU and NIC           */
    HINT_MAX
} HintType_t;

void hpcat_hint_global_check(Hpcat *hpcat, Task *task);
void hpcat_hint_task_check(Hpcat *hpcat, Task *task);
void hpcat_hint_format(char *output_str, const char detected_hints);

#endif /* HPCAT_HINT_H */
