![HPCAT](https://github.com/HewlettPackard/hpcat/blob/main/img/hpcat.png?raw=true)

**HPCAT** provides a comprehensive visualization of system topology and resource
affinities tailored for High Performance Computing (HPC) applications. It highlights
the relationships between **NUMA nodes**, **CPU cores**, **Hardware threads**,
**Network interfaces** and **GPU devices**.

It reports key runtime affinities, including:

* **Fabric (group ID)** (cross-group communication incurs extra switch hops - currently supports HPE Slingshot with Dragonfly topology only)
* **MPI tasks**
* **OpenMP threads** (automatically enabled when `OMP_NUM_THREADS` is set)
* **Accelerators** (automatically enabled if AMD, Intel or NVIDIA GPUs are detected)
* **Network Interface Cards (NICs)** (available with *Cray MPICH*, starting from 2 nodes)

The output format is a human-readable, condensed table, but *YAML* is also available
as an option.

> [!NOTE]
> A key feature of this application is its use of dynamically linked modules
> to retrieve information about system accelerators. This design allows a
> single binary to run seamlessly across different cluster partitions (regardless
> of whether accelerators are present) making it easier to deploy and maintain
> a consistent user experience across the entire HPC system.

![HPCAT Output](https://github.com/HewlettPackard/hpcat/blob/main/img/hpcat-main-example.png?raw=true)


Table of Contents
-----------------

1. [Dependencies](#dependencies)
1. [Installation](#installation)
1. [Usage](#usage)
1. [Arguments](#arguments)
1. [Examples](#examples)
   - [CPU nodes (AMD EPYC Genoa)](#antero-amd-cpus-epyc-genoa-nodes)
   - [GPU nodes (AMD Instinct MI250X)](#bardpeak-amd-gpus-instinct-mi250x-nodes)
   - [GPU nodes (Intel Max 1550)](#exascale-compute-blade-intel-gpus-max-1550-nodes)
   - [GPU nodes (NVIDIA A100)](#grizzlypeak-blade-nvidia-gpus-a100-nodes)
1. [Scalability](#scalability)
1. [Future Work](#future-work)


Dependencies
------------

* **AMD ROCm** (Optional, for AMD GPUs)
* **Intel OneAPI Level Zero** (Optional, for Intel GPUs)
* **NVIDIA NVML** (Optional, for NVIDIA GPUs)
* **MPI**
* **[hwloc](https://github.com/open-mpi/hwloc)** (built with **HPCAT**)
* **[libfort](https://github.com/seleznevae/libfort)** (built with **HPCAT**)


Installation
------------

**HPCAT** uses git submodules and CMake with a configure wrapper.

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
> The tool will be installed in the *install* subdirectory by default. You may want
> to use  `./configure --prefix=<destination_path>` to define your own destination
> path (or directly use CMake and not the configure wrapper).
> If you decide to move the tool to a different directory, make sure that the
> dynamic library modules (in <destination_path>/lib) are also moved.
> Otherwise, the module(s) will be ignored.


> [!TIP]
> Compilation of each GPU module can also be skipped (check `./configure --help`).

Usage
-----

**HPCAT** should be launched in the same manner as your application. With Slurm,
you can add an additional srun command using the same arguments to match the
properties of the original srun line, including resource allocation and bindings.
If you are using a wrapper script with your application, be sure to include it to
capture all affinity and binding properties.

A convenient approach is to use a variable to share the job scheduler arguments.
For example, with Slurm in your batch script:

    SLURM_ARGS="-N 2 -n 16 -c 16 --hint=nomultithread"
    srun $SLURM_ARGS bin/hpcat
    srun $SLURM_ARGS <app>


Arguments
---------

**HPCAT** accepts the following arguments:

    -c, --enable-color-dark    Using colors (dark terminal)
        --disable-accel        Don't display GPU affinities
        --disable-fabric       Don't display fabric group ID
        --disable-nic          Don't display Network affinities
        --disable-omp          Don't display OpenMP affinities
        --enable-color-light   Using colors (light terminal)
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

    OMP_NUM_THREADS=2 srun -p antero -N 2 --tasks-per-node=8 -c 24 --hint=nomultithread hpcat -c --no-banner

![HPCAT Antero](https://github.com/HewlettPackard/hpcat/blob/main/img/hpcat-antero-example.png?raw=true)


### Bardpeak [AMD GPUs Instinct MI250X] nodes:

    MPICH_OFI_NIC_POLICY=NUMA srun -p bardpeak -N 2 --tasks-per-node=8 -c 8 --hint=nomultithread ./gpu-affinity.sh hpcat -c --no-banner

![HPCAT Bardpeak](https://github.com/HewlettPackard/hpcat/blob/main/img/hpcat-bardpeak-example.png?raw=true)


### Exascale Compute Blade [Intel GPUs Max 1550] nodes:

    MPICH_OFI_NIC_POLICY=NUMA srun -p ecb -N 2 --tasks-per-node=12 -c 8 --hint=nomultithread ./gpu-affinity.sh hpcat -c --no-banner

![HPCAT ECB](https://github.com/HewlettPackard/hpcat/blob/main/img/hpcat-ecb-example.png?raw=true)


### Grizzlypeak Blade [NVIDIA GPUs A100] nodes:

    MPICH_OFI_NIC_POLICY=NUMA srun -p griz512 -N 2 --tasks-per-node=4 -c 16 --hint=nomultithread ./gpu-affinity.sh hpcat -c --no-banner

![HPCAT Grizzlypeak](https://github.com/HewlettPackard/hpcat/blob/main/img/hpcat-grizzlypeak-example.png?raw=true)


Scalability
-----------

`HPCAT` maintains a low runtime. Most of the execution time is spent in library
operations, such as initializing the GPU framework to retrieve locality information,
scanning the system topology with hwloc, or displaying the tabular output on
the first rank.

The plot below shows the execution time on LUMI as a function of the number of
compute nodes (each equipped with MI250x AMD GPUs). The configuration uses 8 MPI
ranks per node, with one GPU (GCD) assigned to each rank. At 256 nodes, a total
of 2048 MPI tasks are launched. Colors are disabled in this run:

![Scalability up to 256 nodes](https://github.com/HewlettPackard/hpcat/blob/main/img/hpcat-scalability.png?raw=true)


Future Work
-----------

- [ ] Provide support for NIC affinities with other MPI distributions (if possible).
- [ ] Display warning tips to highlight potential performance issues resulting from suboptimal bindings.
