<div align="center">

![HPCAT](https://github.com/HewlettPackard/hpcat/blob/main/img/hpcat.png?raw=true)
![GitHub Tag](https://img.shields.io/github/v/tag/HewlettPackard/hpcat?style=for-the-badge&label=version&color=0xe0af48)

</div>

**HPC Affinity Tracker** (**HPCAT**) provides a comprehensive visualization of
system topology and resource affinities tailored for High Performance Computing
(HPC) applications. It highlights the relationships between **NUMA nodes**,
**CPU cores**, **Hardware threads**, **Network interfaces** and **GPU devices**.

It reports key runtime affinities, including:

* **Fabric (group ID)** (cross-group communication incurs extra switch hops - currently supports HPE Slingshot with Dragonfly topology only)
* **MPI tasks**
* **OpenMP threads** (automatically enabled when `OMP_NUM_THREADS` is set)
* **Accelerators** (automatically enabled if AMD, Intel or NVIDIA GPUs are detected)
* **Network Interface Cards (NICs)** (available with *Cray MPICH*, starting from 2 nodes)

The output format is a human-readable, condensed table. By default, `HPCAT`
displays hints in the footer of the tabular output, highlighting detected binding or
affinity issues that may lead to performance degradation.

*YAML* output is also available as an option.

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
1. [Environment](#environment)
1. [Scalability](#scalability)
1. [Changelog](#changelog)
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

**HPCAT** uses Git submodules and CMake, along with a wrapper script that simplifies
configuration and emulates the behavior of autotools. To build the project,
follow these steps:

    git clone --recurse-submodules https://github.com/HewlettPackard/hpcat.git
    cd hpcat
    ./configure
    make -j 10
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
> dynamic library modules (in `<destination_path>/lib`) are also moved.
> Otherwise, the module(s) will be ignored.

> [!TIP]
> * You can skip compilation of individual GPU modules using flags like `--disable-gpu-amd`, `--disable-gpu-intel`, or `--disable-gpu-nvidia`.
> * To specify a different compiler, set the `CC` environment variable before running the configuration script (e.g., `CC=<path_to_mpicc> ./configure`).
> * Run `./configure --help` for a full list of available options.


Usage
-----

For your convenience, a module file is installed to enable quick access to **HPCAT**.
To use it, please follow these steps (adjust the path if a custom installation prefix was used):

    module use install/share/modulefiles
    module load hpcat


**HPCAT** should be launched in the same manner as your application. With Slurm,
you can add an additional srun command using the same arguments to match the
properties of the original srun line, including resource allocation and bindings.
If you are using a wrapper script with your application, be sure to include it to
capture all affinity and binding properties.

A convenient approach is to use a variable to share the job scheduler arguments.
For example, with Slurm in your batch script:

    SLURM_ARGS="-N 2 -n 16 -c 16 --hint=nomultithread"
    srun $SLURM_ARGS hpcat
    srun $SLURM_ARGS <app>


Arguments
---------

**HPCAT** accepts the following arguments:

    -c, --enable-color-dark    Using colors (dark terminal)
        --disable-accel        Don't display GPU affinities
        --disable-fabric       Don't display fabric group ID
        --disable-hints        Don't display hints
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

To obtain this list, use the `--help` option.
For additional information, please refer to the man page.


Examples
--------

### Antero `[AMD CPUs EPYC Genoa]` nodes:

    OMP_NUM_THREADS=2 srun -p antero -N 2 --tasks-per-node=8 -c 24 --hint=nomultithread hpcat -c --no-banner

![HPCAT Antero](https://github.com/HewlettPackard/hpcat/blob/main/img/hpcat-antero-example.png?raw=true)


### Bardpeak `[AMD GPUs Instinct MI250X]` nodes:

    MPICH_OFI_NIC_POLICY=NUMA srun -p bardpeak -N 2 --tasks-per-node=8 -c 8 --hint=nomultithread ./gpu-affinity.sh hpcat -c --no-banner

![HPCAT Bardpeak](https://github.com/HewlettPackard/hpcat/blob/main/img/hpcat-bardpeak-example.png?raw=true)


### Exascale Compute Blade `[Intel GPUs Max 1550]` nodes:

    MPICH_OFI_NIC_POLICY=NUMA srun -p ecb -N 2 --tasks-per-node=12 -c 8 --hint=nomultithread ./gpu-affinity.sh hpcat -c --no-banner

![HPCAT ECB](https://github.com/HewlettPackard/hpcat/blob/main/img/hpcat-ecb-example.png?raw=true)


### Grizzlypeak Blade `[NVIDIA GPUs A100]` nodes:

    MPICH_OFI_NIC_POLICY=NUMA srun -p griz512 -N 2 --tasks-per-node=4 -c 16 --hint=nomultithread ./gpu-affinity.sh hpcat -c --no-banner

![HPCAT Grizzlypeak](https://github.com/HewlettPackard/hpcat/blob/main/img/hpcat-grizzlypeak-example.png?raw=true)


Environment
-----------

> [!IMPORTANT]
> When using Slingshot with Cray MPICH, the feature that aligns NIC affinity with
> GPU affinity is not natively supported. This is because the Cray MPICH feature
> relies on specific libraries that are not linked during compilation to preserve
> the modularity of `HPCAT`. However, setting the environment variable `MPICH_OFI_NIC_POLICY`
> to `GPU` makes the tool emulate NIC affinity to match GPU NUMA affinity.


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


Changelog
---------

For a detailed list of changes, please see the [CHANGELOG.md](CHANGELOG.md).


Future Work
-----------

- [ ] Provide support for NIC affinities with other MPI distributions (if possible).
- [ ] Display warning tips to highlight potential performance issues resulting from suboptimal bindings.
