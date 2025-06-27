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
* accel_nvml.c: Dynamic library for NVIDIA accelerators.
*
* URL       https://github.com/HewlettPackard/hpcat
******************************************************************************/

#include <stdio.h>
#include <stdbool.h>
#include <hwloc.h>
#include <nvml.h>
#include "common.h"

bool nvml_is_init = false;

static int nvml_init(void)
{
    if (nvml_is_init)
        return 0;

    if (nvmlInit() != NVML_SUCCESS)
        return -1;

    nvml_is_init = true;
    return 0;
}

/**
 * Retrieve how many AMD devices are available from this context
 *
 * @return                   Quantity of devices found or -1
 */
int hpcat_accel_count(void)
{
    unsigned int dev_count = 0;
    if (nvml_init() != 0)
        return -1;

    return (nvmlDeviceGetCount(&dev_count) == NVML_SUCCESS) ? (int)dev_count : -1;
}

static int get_device_id_array(int *ids, int *dev_count)
{
    int count = 0;

    char *visible_env = getenv("CUDA_VISIBLE_DEVICES");
    if (visible_env == NULL)
    {
        count = hpcat_accel_count();
        for (int i = 0; i < count; i++)
            ids[i] = i;
    }
    else
    {
        hwloc_bitmap_t bitmap = hwloc_bitmap_alloc();
        if (bitmap == NULL)
        {
            printf("Failed to allocate bitmap\n");
            return -1;
        }

        if (strlist_to_bitmap(bitmap, visible_env) != 0)
        {
            printf("Failed to convert string list to bitmap\n");
            return -1;
        }

        count = hwloc_bitmap_weight(bitmap);
        if (count > MAX_DEVICES)
        {
            printf("Device count is larger than the upper limit\n");
            return -1;
        }

        ids[0] = hwloc_bitmap_first(bitmap);

        for (int i = 1; i < count; i++)
            ids[i] = hwloc_bitmap_next(bitmap, ids[i - 1]);

        hwloc_bitmap_free(bitmap);
    }

    *dev_count = count;
    return 0;
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
    int ids[MAX_DEVICES] = {0};
    int dev_count = 0;

    /* List of devices */
    int ret = get_device_id_array(ids, &dev_count);
    if (ret != 0)
        return -1;

    if (dev_count <= 0)
        return -1;

    int max_size = max_buff_size - 1;

    for (int i = 0; i < dev_count; i++)
    {
        char pci[PCI_STR_MAX] = { 0 };

        nvmlDevice_t device;
        nvmlPciInfo_t pci_info;

        nvmlReturn_t res = nvmlDeviceGetHandleByIndex(ids[i], &device);
        if (NVML_SUCCESS != res)
        {
            printf("Failed to get handle for device %u: %s\n", ids[i], nvmlErrorString(res));
            return -1;
        }

        res = nvmlDeviceGetPciInfo(device, &pci_info);
        if (NVML_SUCCESS != res) {
            printf("Failed to get PCI info for device %u: %s\n", ids[i], nvmlErrorString(res));
            return -1;
        }

        snprintf(pci, PCI_STR_MAX - 1, "%s[%01x:%02x]", (buff[0] == '\0') ? "" : ",",
                                                        pci_info.domain, pci_info.bus);
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
    char *visible_env = getenv("CUDA_VISIBLE_DEVICES");

    if (visible_env == NULL)
    {
        set_first_bits_bitmap(bitmap, hpcat_accel_count());
        return 0;
    }
    else
        return strlist_to_bitmap(bitmap, visible_env);
}

/**
 * Retrieve the NUMA affinity of the first GPU
 *
 * @return                      Success: NUMA node, Error: -1
 */
int hpcat_accel_numa_first(void)
{
    const int dev_count = hpcat_accel_count();
    if (dev_count <= 0)
        return -1;

    nvmlDevice_t device;
    nvmlPciInfo_t pci;

    nvmlReturn_t res = nvmlDeviceGetHandleByIndex(ids[0], &device);
    if (NVML_SUCCESS != res)
    {
        printf("Failed to get handle for device 0: %s\n", nvmlErrorString(res));
        return -1;
    }

    res = nvmlDeviceGetPciInfo(device, &pci);
    if (NVML_SUCCESS != res)
    {
        printf("Failed to get PCI info for device 0: %s\n", nvmlErrorString(res));
        return -1;
    }

    return get_device_numa_affinity(pci.domain, pci.bus);
}

/**
 * Retrieve a bitmap representing NUMA affinities of all detected accelerators
 *
 * @param   numa_affinity[out]  Preallocated hwloc bitmap
 * @return                      Success: 0, Error: -1
 */
int hpcat_accel_numa_bitmap(hwloc_bitmap_t numa_affinity)
{
    int ids[MAX_DEVICES] = {0};
    int dev_count = 0;

    /* List of devices */
    int ret = get_device_id_array(ids, &dev_count);
    if (ret != 0)
        return -1;

    if (dev_count <= 0)
        return -1;

    for (int i = 0; i < dev_count; i++)
    {
        nvmlDevice_t device;
        nvmlPciInfo_t pci;

        nvmlReturn_t res = nvmlDeviceGetHandleByIndex(ids[i], &device);
        if (NVML_SUCCESS != res)
        {
            printf("Failed to get handle for device %u: %s\n", ids[i], nvmlErrorString(res));
            return -1;
        }

        res = nvmlDeviceGetPciInfo(device, &pci);
        if (NVML_SUCCESS != res)
        {
            printf("Failed to get PCI info for device %u: %s\n", ids[i], nvmlErrorString(res));
            return -1;
        }

        const int numa_node = get_device_numa_affinity(pci.domain, pci.bus);
        if (numa_node == -1)
            return -1;

        hwloc_bitmap_set(numa_affinity, numa_node);
    }

    return 0;
}
