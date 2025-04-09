![HPCAT Badge](https://github.com/HewlettPackard/hpcat/blob/main/img/hpcat.png?raw=true)

This application is designed to display **NUMA**, **CPU core**, **Hardware threads**,
**Network interfaces** and **GPU affinities** within the context of HPC applications.

It provides reports on **MPI** tasks, **OpenMP** (automatically enabled if
`OMP_NUM_THREADS` is set), **accelerators** (automatically enabled if GPUs are
detected), and **NICs** (*Cray MPICH* only, starting from 2 nodes).

The output format is a human-readable, condensed table, but *YAML* is also available
as an option.

> [!NOTE]
> The application uses dynamic linking modules to retrieve information about
> accelerators, allowing the same binary to be used across different partitions,
> whether or not accelerators are present.

![HPCAT Output](https://github.com/HewlettPackard/hpcat/blob/main/img/hpcat-main-example.png?raw=true)


Table of Contents
-----------------

1. [Dependencies](#dependencies)
1. [Installation](#installation)
1. [Usage](#usage)
1. [Examples](#examples)
   - [CPU nodes (AMD EPYC Genoa)](#antero-amd-cpus-epyc-genoa-nodes)
   - [GPU nodes (AMD Instinct MI250X)](#bardpeak-amd-gpus-instinct-mi250x-nodes)
   - [GPU nodes (Intel Max 1550)](#exascale-compute-blade-intel-gpus-max-1550-nodes)
   - [GPU nodes (NVIDIA A100)](#grizzlypeak-blade-nvidia-gpus-a100-nodes)


Dependencies
------------

* **AMD ROCm** (Optional, for AMD GPUs)
* **Intel OneAPI Level Zero** (Optional, for Intel GPUs)
* **NVIDIA NVML** (Optional, for NVIDIA GPUs)
* **MPI**
* **[hwloc](https://github.com/open-mpi/hwloc)** (built with HPCAT)
* **[libfort](https://github.com/seleznevae/libfort)** (built with HPCAT)


Installation
------------

HPCAT uses git submodules and CMake with a configure wrapper.

    git submodule update --init --recursive
    ./configure
    make
    make install


> [!IMPORTANT]
> GPU modules are automatically built when the proper libraries are found. But:
> * For NVIDIA GPU module, `CUDA_HOME` environment variable should be set.
> * For Intel GPU module, `ONEAPI_ROOT` environment variable should be set
> (`source /opt/intel/oneapi/setvars.sh` can be used to set it).


> [!CAUTION]
> The tool will be installed in the *bin* subdirectory by default. You may want
> to use  `./configure --prefix=<destination_path>` to define your own destination
> path (or directly use CMake and not the configure wrapper).
> If you decide to move the tool to a different directory, make sure that the
> dynamic library modules are stored in the same directory as the *hpcat* binary.
> Otherwise, the module(s) will be ignored.


> [!TIP]
> Compilation of each GPU module can also be skipped (check `./configure --help`).


Usage
-----

    % hpcat [ARGS...]

Arguments are :

        --disable-accel        Don't display GPU affinities
        --disable-color        Don't use colors in the output
        --disable-nic          Don't display Network affinities
        --disable-omp          Don't display OpenMP affinities
        --enable-color         Using colors in the bash output
        --enable-omp           Display OpenMP affinities
        --no-banner            Don't display header/footer
    -v, --verbose              Make the operations talkative
    -y, --yaml                 YAML output
    -?, --help                 Give this help list
        --usage                Give a short usage message
    -V, --version              Print program version


Examples
--------

### Antero [AMD CPUs EPYC Genoa] nodes:

    OMP_NUM_THREADS=2 srun -p antero -N2 --tasks-per-node=8 -c 24 --hint=nomultithread --pty bin/hpcat --no-banner

![HPCAT Antero](https://github.com/HewlettPackard/hpcat/blob/main/img/hpcat-antero-example.png?raw=true)


### Bardpeak [AMD GPUs Instinct MI250X] nodes:

    MPICH_OFI_NIC_POLICY=NUMA srun -p bardpeak -N2 --tasks-per-node=8 -c 8 --hint=nomultithread --pty ./gpu-affinity.sh bin/hpcat --no-banner

![HPCAT Bardpeak](https://github.com/HewlettPackard/hpcat/blob/main/img/hpcat-bardpeak-example.png?raw=true)


### Exascale Compute Blade [Intel GPUs Max 1550] nodes:

    MPICH_OFI_NIC_POLICY=NUMA srun -p ecb -N 2 --tasks-per-node=12 -c 8 --hint=nomultithread --pty ./gpu-affinity.sh bin/hpcat --no-banner

![HPCAT ECB](https://github.com/HewlettPackard/hpcat/blob/main/img/hpcat-ecb-example.png?raw=true)


### Grizzlypeak Blade [NVIDIA GPUs A100] nodes:

    MPICH_OFI_NIC_POLICY=NUMA srun -p griz512 -N 2 --tasks-per-node=4 -c 16 --hint=nomultithread --pty ./gpu-affinity.sh bin/hpcat --no-banner

![HPCAT Grizzlypeak](https://github.com/HewlettPackard/hpcat/blob/main/img/hpcat-grizzlypeak-example.png?raw=true)
