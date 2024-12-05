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
* common.h: Common functions
*
* URL       https://github.com/HewlettPackard/hpcat
******************************************************************************/

#ifndef HPCAT_COMMON_H
#define HPCAT_COMMON_H

#include <stdio.h>

#define PCI_STR_MAX    32

/**
 * Retrieve NUMA affinity of a device based on its PCIe address
 *
 * @param   domain[in]       PCIe Domain of the device
 * @param   bus[in]          PCIe Bus Address of the device
 * @return                   NUMA node affinity or -1
 */
static inline int get_device_numa_affinity(const int domain, const int bus)
{
    int numa_node = -1;
    char numa_file[PATH_MAX];
    snprintf(numa_file, PATH_MAX - 1,
                    "/sys/class/pci_bus/%04x:%02x/device/numa_node", domain, bus);

    FILE* file = fopen(numa_file, "r");
    if (file == NULL)
        return -1;

    int ret = fscanf(file, "%d", &numa_node);
    fclose(file);
    if (ret == EOF || ret == 0)
        return -1;

    return numa_node;
}

#endif /* HPCAT_COMMON_H */
