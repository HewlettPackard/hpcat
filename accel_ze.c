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
* accel_ze.c: Dynamic library for Intel accelerators.
*
* URL       https://github.com/HewlettPackard/hpcat
******************************************************************************/

#include <stdio.h>
#include <stdbool.h>
#include <hwloc.h>
#include <level_zero/ze_api.h>
#include <level_zero/zes_api.h>
#include "common.h"

zes_driver_handle_t *ze_drivers = NULL;
zes_device_handle_t *ze_devices = NULL;
int ze_devices_count = 0;
bool ze_is_init = false;

static int ze_init(void)
{
    if (ze_is_init)
        return 0;

    /* Enable driver initialization and dependencies for system management */
    if (setenv("ZES_ENABLE_SYSMAN", "1", 1) != 0)
        goto error;

    /* Initialize OneAPI Level Zero */
    if (zeInit(ZE_INIT_FLAG_GPU_ONLY) != ZE_RESULT_SUCCESS)
        goto error;

    /* Retrieve OneAPI Level Zero drivers */
    uint32_t driver_count = 0;
    ze_result_t ret = zeDriverGet(&driver_count, NULL);
    if ((ret != ZE_RESULT_SUCCESS) || (driver_count == 0))
        goto error;

    ze_drivers = malloc(driver_count * sizeof(ze_driver_handle_t));
    if (ze_drivers == NULL)
        goto error;

    ret = zeDriverGet(&driver_count, ze_drivers);
    if (ret != ZE_RESULT_SUCCESS)
        goto error;

    /* Retrieve OneAPI Level Zero devices */
    ret = zeDeviceGet(ze_drivers[0], &ze_devices_count, NULL);
    if (ret != ZE_RESULT_SUCCESS)
    {
        const char *estring;
        zeDriverGetLastErrorDescription(ze_drivers[0], &estring);
        goto error;
    }

    if (ze_devices_count == 0)
        goto error;

    ze_devices = malloc(ze_devices_count * sizeof(zes_device_handle_t));
    if (ze_devices == NULL)
        goto error;

    ret = zeDeviceGet(ze_drivers[0], &ze_devices_count, ze_devices);
    if (ret != ZE_RESULT_SUCCESS)
    {
        const char *estring;
        zeDriverGetLastErrorDescription(ze_drivers[0], &estring);
        goto error;
    }

    ze_is_init = true;

    return 0;

error:
    if (ze_drivers != NULL)
        free(ze_drivers);

    if (ze_devices != NULL)
        free(ze_devices);

    return -1;
}

/**
 * Retrieve how many Intel devices are available from this context
 *
 * @return                   Quantity of devices found or -1
 */
int hpcat_accel_count(void)
{
    int count = 0;

    if (ze_init() != 0)
        return -1;

    for (int i = 0; i < ze_devices_count; i++)
    {
        zes_device_handle_t dev = ze_devices[i];
        zes_device_properties_t dev_props;
        dev_props.stype = ZE_STRUCTURE_TYPE_DEVICE_PROPERTIES;

        if ((zesDeviceGetProperties(dev, &dev_props) == ZE_RESULT_SUCCESS) &&
            strstr(dev_props.brandName, "Intel"))
            count++;
    }

    return count;
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

    uint32_t last_domain = 0, last_bus = 0;

    for (int i = 0; i < ze_devices_count; i++)
    {
        char pci[PCI_STR_MAX] = { 0 };

        zes_device_handle_t dev = ze_devices[i];
        zes_device_properties_t dev_props;
        dev_props.stype = ZE_STRUCTURE_TYPE_DEVICE_PROPERTIES;

        if ((zesDeviceGetProperties(dev, &dev_props) != ZE_RESULT_SUCCESS) ||
            !strstr(dev_props.brandName, "Intel"))
            continue;

        /* Retrieving the PCIe address */
        zes_pci_properties_t pci_prop;
        ze_result_t ret = zesDevicePciGetProperties(dev, &pci_prop);
        if (ret != ZE_RESULT_SUCCESS)
        {
            const char *estring;
            zeDriverGetLastErrorDescription(ze_drivers[0], &estring);
            printf("Failed to get PCI info for device %u: %s\n", i, estring);
            return -1;
        }

        zes_pci_address_t *addr = &pci_prop.address;

        /* Avoid consecutive duplicates (GPU tiles on the same package) */
        if ((addr->domain == last_domain) && (addr->bus == last_bus))
            continue;

        snprintf(pci, PCI_STR_MAX - 1, "%s%01x:%02x", (buff[0] == '\0') ? "" : ",",
                                                      addr->domain, addr->bus);

        strncat(buff, pci, max_size);
        max_size -= strlen(pci);
        if (max_size <= 0)
            return -1;

        last_domain = addr->domain;
        last_bus = addr->bus;
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
    char *visible_env = getenv("ZE_AFFINITY_MASK");

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

    for (int i = 0; i < ze_devices_count; i++)
    {
        zes_device_handle_t dev = ze_devices[i];
        zes_device_properties_t dev_props;
        dev_props.stype = ZE_STRUCTURE_TYPE_DEVICE_PROPERTIES;

        if ((zesDeviceGetProperties(dev, &dev_props) != ZE_RESULT_SUCCESS) ||
            !strstr(dev_props.brandName, "Intel"))
            continue;

        /* Retrieving the PCIe address */
        zes_pci_properties_t pci_prop;
        ze_result_t ret = zesDevicePciGetProperties(dev, &pci_prop);
        if (ret != ZE_RESULT_SUCCESS)
        {
            const char *estring;
            zeDriverGetLastErrorDescription(ze_drivers[0], &estring);
            printf("Failed to get PCI info for device %u: %s\n", i, estring);
            return -1;
        }

        const int numa_node = get_device_numa_affinity(pci_prop.address.domain,
                                                       pci_prop.address.bus);
        if (numa_node == -1)
            return -1;

        hwloc_bitmap_set(numa_affinity, numa_node);
    }

    return 0;
}
