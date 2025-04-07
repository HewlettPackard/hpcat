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
* settings.c: Application specific settings and user defined parameters.
*
* URL       https://github.com/HewlettPackard/hpcat
******************************************************************************/

#include <stdio.h>
#include <stdint.h>
#include <stdlib.h>
#include <argp.h>
#include "settings.h"

const char *argp_program_version = HPCAT_VERSION;
const char *argp_program_bug_address = HPCAT_CONTACT;

/* Program documentation */
static char doc[] = "This application is designed to display NUMA and CPU affinities in the context "
                    "of HPC applications. Details about, MPI tasks, OpenMP (automatically enabled if "
                    "OMP_NUM_THREADS is set), accelerators (automatically enabled if GPUs are detected) "
                    "and network interfaces (Cray MPICH only, starting from 2 nodes) are reported. "
                    "The application only shows detected affinities but parameters can be used to "
                    "force enabling/disabling details. It accepts the following arguments:";

/* A description of the arguments we accept (in addition to the options) */
static char args_doc[] = " ";

/* Options */
static struct argp_option options[] =
{
    {"enable-omp",      11,  0,         0,  "Display OpenMP affinities"},
    {"disable-omp",     21,  0,         0,  "Don't display OpenMP affinities"},
    {"disable-accel",   22,  0,         0,  "Don't display GPU affinities"},
    {"disable-nic",     23,  0,         0,  "Don't display Network affinities"},
    {"no-banner",       31,  0,         0,  "Don't display header/footer"},
    {"verbose",         'v', 0,         0,  "Make the operations talkative"},
    {"yaml",            'y', 0,         0,  "YAML output"},
    {0}
};

/* Parse a single option */
static error_t parse_opt(int key, char *arg, struct argp_state *state)
{
    HpcatSettings_t *settings = (HpcatSettings_t *)state->input;

    switch (key)
    {
        case  11:
            settings->enable_omp = true;
            break;
        case  21:
            settings->enable_omp = false;
            break;
        case  22:
            settings->enable_accel = false;
            break;
        case  23:
            settings->enable_nic = false;
            break;
        case  31:
            settings->enable_banner = false;
            break;
        case 'v':
            settings->enable_verbose = true;
            break;
        case 'y':
            settings->output_type = YAML;
            break;
        case ARGP_KEY_END:
            break;
        default:
            return ARGP_ERR_UNKNOWN;
    }

    return 0;
}

/* Argp parser */
static struct argp argp = { options, parse_opt, args_doc, doc };

/**
 * Initialize settings / parsing arguments
 *
 * @param   argc[in]          Amount of arguments
 * @param   argv[in]          Array of arguments
 * @param   hpcat_args[out]   Application settings structure
 */
void hpcat_settings_init(int argc, char *argv[], HpcatSettings_t *hpcat_settings)
{
    /* Set defaults and auto-detect */
    hpcat_settings->output_type    = STDOUT;

    hpcat_settings->enable_accel   = true;
    hpcat_settings->enable_banner  = true;
    hpcat_settings->enable_nic     = true;
    hpcat_settings->enable_verbose = false;

    char *omp_env = getenv("OMP_NUM_THREADS");
    hpcat_settings->enable_omp = (omp_env != NULL) && (atoi(omp_env) > 1);

    argp_parse(&argp, argc, argv, 0, 0, hpcat_settings);
}
