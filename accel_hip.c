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
* accel_hip.c: Dynamic library for AMD accelerators.
*
* URL       https://github.com/HewlettPackard/hpcat
******************************************************************************/

#include <stdio.h>
#include <hwloc.h>
#include <hip/hip_runtime.h>
#include "common.h"

/**
 * Retrieve how many AMD devices are available from this context
 *
 * @return                   Quantity of devices found or -1
 */
int hpcat_accel_count(void)
{
    int dev_count = 0;
    return (hipGetDeviceCount(&dev_count) == hipSuccess) ? dev_count : -1;
}

/**
 *  Format a list (comma separated) of the PCIe addresses of all
 *  accelerators found.
 *
 * @param   buff[out]          Output buffer for the list of addresses
 * @param   max_buff_size[in]  Size of the buffer
 * @return                     Success: 0, Error: -1
 */
int hpcat_accel_pciaddr_list_str(char *buff, const int max_buff_size)
{
    const int dev_count = hpcat_accel_count();
    if (dev_count <= 0)
        return -1;

    int max_size = max_buff_size - 1;

    for (int i = 0; i < dev_count; i++)
    {
        char pci[PCI_STR_MAX] = { 0 };
        struct hipDeviceProp_t prop;
        if (hipGetDeviceProperties(&prop, i) != hipSuccess)
            return -1;

        if (buff[0] != '\0')
        {
            strncat(buff, ",", max_size);
            max_size--;
        }

        /* XXX: Use PCIe DomainId with MI300A (gfx942) */
        if (strncmp(prop.gcnArchName, "gfx942:", 7) == 0)
            snprintf(pci, PCI_STR_MAX - 1, "%02x", prop.pciDomainID);
        else
            snprintf(pci, PCI_STR_MAX - 1, "%02x", prop.pciBusID);

        strncat(buff, pci, max_size);
        max_size -= strlen(pci);
        if (max_size <= 0)
            return -1;
    }

    return 0;
}

/**
 * Retrive a list of visible devices in a bitmap
 *
 * @param   bitmap[out]   Preallocated hwloc bitmap
 * @return                Success: 0, Error: -1
 */
int hpcat_accel_visible_bitmap(hwloc_bitmap_t bitmap)
{
    char *visible_env = getenv("ROCR_VISIBLE_DEVICES");

    if (visible_env == NULL)
    {
        set_first_bits_bitmap(bitmap, hpcat_accel_count());
        return 0;
    }
    else
        return strlist_to_bitmap(bitmap, visible_env);
}

/**
 * Retrieve a bitmap representing NUMA affinities of all detected accelerators
 *
 * @param   numa_affinity[out]  Preallocated hwloc bitmap
 * @return                      Success: 0, Error: -1
 */
int hpcat_accel_numa_bitmap(hwloc_bitmap_t numa_affinity)
{
    const int dev_count = hpcat_accel_count();
    if (dev_count <= 0)
        return -1;

    for (int i = 0; i < dev_count; i++)
    {
        struct hipDeviceProp_t prop;
        if (hipGetDeviceProperties(&prop, i) != hipSuccess)
            return -1;

        const int numa_node = get_device_numa_affinity(prop.pciDomainID, prop.pciBusID);
        if (numa_node == -1)
            return -1;

        hwloc_bitmap_set(numa_affinity, numa_node);
    }

    return 0;
}
