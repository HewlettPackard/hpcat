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
* hints.c: Hint detection and management
*
* URL       https://github.com/HewlettPackard/hpcat
******************************************************************************/

#include "hint.h"
#include "common.h"

#define GET_BIT(bitmap, bit) ((bitmap >> bit) & 1)
#define SET_BIT(bitmap, bit) (bitmap | (1 << bit))

static const char * const hint_str[] =
{
    [HINT_SHARED_CORES]             = "Tasks share the same CPU core(s)",
    [HINT_MULTIPLE_NUMA_NODES]      = "Task(s) span multiple NUMA nodes",
    [HINT_DIFFERENT_CPU_GPU_NUMA]   = "Task(s) have different CPU and GPU NUMA affinities",
    [HINT_DIFFERENT_CPU_NIC_NUMA]   = "Task(s) have different CPU and NIC NUMA affinities",
    [HINT_DIFFERENT_GPU_NIC_NUMA]   = "Task(s) have different GPU and NIC NUMA affinities",
};

static inline void hint_set(char *detected_hints, const HintType_t type)
{
    *detected_hints = SET_BIT(*detected_hints, type);
}

static inline bool hint_is_set(const char detected_hints, const HintType_t type)
{
    return (bool)GET_BIT(detected_hints, type);
}

static inline bool hint_is_empty(const char detected_hints)
{
    return (detected_hints == 0);
}

static hwloc_bitmap_t alloc_bitmap(void)
{
    hwloc_bitmap_t bitmap = hwloc_bitmap_alloc();
    if (bitmap == NULL)
        FATAL("Error: Unable to allocate temporary bitmap for hints. Exiting.\n");

    return bitmap;
}

/**
 * Performs a cross-task check (must be called on all tasks).
 * This function also merges all detected binding and affinity issues
 * into the hpcat->detected_hints variable for simplified hint reporting.
 *
 * @param   hpcat[in,out]     Global context
 * @param   task[in,out]      Task context
 */
void hpcat_hint_global_check(Hpcat *hpcat, Task *task)
{
    if (!hpcat->settings.enable_hints)
        return;

    if (task->is_first_rank)
        hpcat->global_cpu_bitmap = alloc_bitmap();

    if(task->is_first_node_rank)
        hwloc_bitmap_zero(hpcat->global_cpu_bitmap);

    /* Check if this task is reusing CPU cores in a node */
    hwloc_bitmap_t task_cpu_bitmap = alloc_bitmap();
    hwloc_bitmap_t tmp_bitmap = alloc_bitmap();

    hwloc_bitmap_from_ulongs(task_cpu_bitmap,
                             task->affinity.core_affinity.num_ulongs,
                             task->affinity.core_affinity.ulongs);

    hwloc_bitmap_and(tmp_bitmap, hpcat->global_cpu_bitmap, task_cpu_bitmap);

    if (hwloc_bitmap_weight(tmp_bitmap) > 1)
        hint_set(&task->detected_hints, HINT_SHARED_CORES);

    hwloc_bitmap_zero(tmp_bitmap);
    hwloc_bitmap_copy(tmp_bitmap, hpcat->global_cpu_bitmap);
    hwloc_bitmap_or(hpcat->global_cpu_bitmap, tmp_bitmap, task_cpu_bitmap);

    hwloc_bitmap_free(task_cpu_bitmap);
    hwloc_bitmap_free(tmp_bitmap);

    if (task->is_last_rank)
        hwloc_bitmap_free(hpcat->global_cpu_bitmap);

    /* Gather all types of detected binding issues */
    hpcat->detected_hints |= task->detected_hints;
}

/**
 * Analyzes a single task and updates its detected_hints field.
 *
 * @param hpcat[in]      Global HPCAT context
 * @param task[in,out]   Task context; updated with detected hints
 */
void hpcat_hint_task_check(Hpcat *hpcat, Task *task)
{
    if (!hpcat->settings.enable_hints)
        return;

    hwloc_bitmap_t tmp_bitmap = alloc_bitmap();

    /* Multi NUMA detection (CPU) */
    hwloc_bitmap_t cpu_numa_bitmap = alloc_bitmap();
    hwloc_bitmap_from_ulongs(cpu_numa_bitmap,
                             task->affinity.numa_affinity.num_ulongs,
                             task->affinity.numa_affinity.ulongs);

    if (hwloc_bitmap_weight(cpu_numa_bitmap) > 1)
        hint_set(&task->detected_hints, HINT_MULTIPLE_NUMA_NODES);

    /* CPU-NIC NUMA mismatch detection */
    hwloc_bitmap_t nic_numa_bitmap = alloc_bitmap();
    if (hpcat->settings.enable_nic)
    {
        hwloc_bitmap_zero(tmp_bitmap);
        hwloc_bitmap_set(nic_numa_bitmap, task->nic.numa_affinity);
        hwloc_bitmap_xor(tmp_bitmap, nic_numa_bitmap, cpu_numa_bitmap);

        if (hwloc_bitmap_weight(tmp_bitmap) > 1)
            hint_set(&task->detected_hints, HINT_DIFFERENT_CPU_NIC_NUMA);
    }

    /* CPU-GPU NUMA mismatch detection */
    hwloc_bitmap_t gpu_numa_bitmap = alloc_bitmap();
    if (hpcat->settings.enable_accel)
    {
        hwloc_bitmap_zero(tmp_bitmap);
        hwloc_bitmap_from_ulongs(gpu_numa_bitmap,
                             task->accel.numa_affinity.num_ulongs,
                             task->accel.numa_affinity.ulongs);
        hwloc_bitmap_xor(tmp_bitmap, gpu_numa_bitmap, cpu_numa_bitmap);

        if (hwloc_bitmap_weight(tmp_bitmap) > 1)
            hint_set(&task->detected_hints, HINT_DIFFERENT_CPU_GPU_NUMA);
    }

    /* GPU-NIC NUMA mismatch detection */
    if (hpcat->settings.enable_nic && hpcat->settings.enable_accel)
    {
        hwloc_bitmap_zero(tmp_bitmap);
        hwloc_bitmap_xor(tmp_bitmap, nic_numa_bitmap, gpu_numa_bitmap);

        if (hwloc_bitmap_weight(tmp_bitmap) > 1)
            hint_set(&task->detected_hints, HINT_DIFFERENT_GPU_NIC_NUMA);
    }

    hwloc_bitmap_free(tmp_bitmap);
    hwloc_bitmap_free(cpu_numa_bitmap);
    hwloc_bitmap_free(nic_numa_bitmap);
    hwloc_bitmap_free(gpu_numa_bitmap);
}

/**
 * Formats a human-readable string based on detected hint flags.
 *
 * @param output_str[out]     Formatted string containing the hint descriptions
 * @param detected_hints[in]  Bitfield containing all detected hint flags
 */
void hpcat_hint_format(char *output_str, const char detected_hints)
{
    output_str[0] = '\0';

    if (hint_is_empty(detected_hints))
        return;

    strcat(output_str, "WARNING(S):");

    for (int i = 0; i < HINT_MAX; i++)
    {
        if (hint_is_set(detected_hints, i))
        {
            strcat(output_str, "\n");
            strcat(output_str, hint_str[i]);
        }
    }
}
