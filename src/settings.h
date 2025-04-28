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
* settings.h: Application specific settings and user defined parameters.
*
* URL       https://github.com/HewlettPackard/hpcat
******************************************************************************/

#ifndef HPCAT_SETTINGS_H
#define HPCAT_SETTINGS_H

#include <stdbool.h>

#define HPCAT_VERSION    "0.7"
#define HPCAT_CONTACT    "https://github.com/HewlettPackard/hpcat"

typedef enum OutputType
{
    STDOUT,
    YAML
} OutputType_t;

typedef struct HpcatSettings
{
    bool          enable_accel;
    bool          enable_nic;
    bool          enable_fabric;
    bool          enable_omp;
    bool          enable_banner;
    bool          enable_color;
    bool          enable_verbose;
    OutputType_t  output_type;
} HpcatSettings_t;

void hpcat_settings_init(int argc, char *argv[], HpcatSettings_t *hpcat_settings);

#endif /* HPCAT_SETTINGS_H */
