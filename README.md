`HPC Affinity Tracker (HPCAT)`
==============================

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


Table of Contents
-----------------

1. [Dependencies](#dependencies)
1. [Installation](#installation)
1. [Usage](#usage)
1. [Examples](#examples)


Dependencies
------------

* **AMD ROCm** (Optional, for AMD GPUs)
* **Intel OneAPI Level Zero** (Optional, for Intel GPUs)
* **NVIDIA NVML** (Optional, for NVIDIA GPUs)
* **MPI**
* **[hwloc](https://github.com/open-mpi/hwloc)**


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
        --disable-nic          Don't display Network affinities
        --disable-omp          Don't display OpenMP affinities
        --enable-omp           Display OpenMP affinities
        --no-banner            Do not display header/footer
    -v, --verbose              Make the operations talkative
    -y, --yaml                 YAML output
    -?, --help                 Give this help list
        --usage                Give a short usage message
    -V, --version              Print program version


Examples
--------

**Bardpeak (AMD GPUs MI250x) nodes:**

    % MPICH_OFI_NIC_POLICY=NUMA srun -p bardpeak -N2 --exclusive --tasks-per-node=8 -c 8 --hint=nomultithread \
    ./gpu-affinity.sh ./hpcat --no-banner
    ╔═════════════════╦══════╦══════════════════════╦═════════════════════════════════════════════╦═════════════════════════════════════╗
    ║            HOST ║  MPI ║         NETWORK      ║                 ACCELERATORS                ║                          CPU        ║
    ║          (NODE) ║ RANK ║  INTERFACE │    NUMA ║      ID │              PCIE ADDR. │    NUMA ║                   CORE ID │    NUMA ║
    ╠═════════════════╬══════╬════════════╪═════════╬═════════╪═════════════════════════╪═════════╬═══════════════════════════╪═════════╣
    ║      pinoak0028 ║      ║            │         ║         │                         │         ║                           │         ║
    ║                 ║    0 ║       cxi2 │       0 ║       4 │                      d1 │       0 ║                       0-7 │       0 ║
    ║                 ║    1 ║       cxi2 │       0 ║       5 │                      d6 │       0 ║                      8-15 │       0 ║
    ║                 ║    2 ║       cxi1 │       1 ║       2 │                      c9 │       1 ║                     16-23 │       1 ║
    ║                 ║    3 ║       cxi1 │       1 ║       3 │                      ce │       1 ║                     24-31 │       1 ║
    ║                 ║    4 ║       cxi3 │       2 ║       6 │                      d9 │       2 ║                     32-39 │       2 ║
    ║                 ║    5 ║       cxi3 │       2 ║       7 │                      de │       2 ║                     40-47 │       2 ║
    ║                 ║    6 ║       cxi0 │       3 ║       0 │                      c1 │       3 ║                     48-55 │       3 ║
    ║                 ║    7 ║       cxi0 │       3 ║       1 │                      c6 │       3 ║                     56-63 │       3 ║
    ╠═════════════════╬══════╬════════════╪═════════╬═════════╪═════════════════════════╪═════════╬═══════════════════════════╪═════════╣
    ║      pinoak0029 ║      ║            │         ║         │                         │         ║                           │         ║
    ║                 ║    8 ║       cxi2 │       0 ║       4 │                      d1 │       0 ║                       0-7 │       0 ║
    ║                 ║    9 ║       cxi2 │       0 ║       5 │                      d6 │       0 ║                      8-15 │       0 ║
    ║                 ║   10 ║       cxi1 │       1 ║       2 │                      c9 │       1 ║                     16-23 │       1 ║
    ║                 ║   11 ║       cxi1 │       1 ║       3 │                      ce │       1 ║                     24-31 │       1 ║
    ║                 ║   12 ║       cxi3 │       2 ║       6 │                      d9 │       2 ║                     32-39 │       2 ║
    ║                 ║   13 ║       cxi3 │       2 ║       7 │                      de │       2 ║                     40-47 │       2 ║
    ║                 ║   14 ║       cxi0 │       3 ║       0 │                      c1 │       3 ║                     48-55 │       3 ║
    ║                 ║   15 ║       cxi0 │       3 ║       1 │                      c6 │       3 ║                     56-63 │       3 ║
    ╚═════════════════╩══════╩════════════╧═════════╩═════════╧═════════════════════════╧═════════╩═══════════════════════════╧═════════╝


**Antero (AMD CPUs Genoa) nodes:**

    % OMP_NUM_THREADS=2 srun -p antero -N2 --tasks-per-node=8 -c 24 --hint=nomultithread ./hpcat --no-banner
    ╔═════════════════╦══════╦══════════════════════╦══════╦═════════════════════════════════════╗
    ║            HOST ║  MPI ║         NETWORK      ║  OMP ║                          CPU        ║
    ║          (NODE) ║ RANK ║  INTERFACE │    NUMA ║   ID ║                   CORE ID │    NUMA ║
    ╠═════════════════╬══════╬════════════╪═════════╬══════╬═══════════════════════════╪═════════╣
    ║      pinoak0051 ║      ║            │         ║      ║                           │         ║
    ║                 ║    0 ║       cxi0 │       2 ║ ---- ║                      0-23 │       0 ║
    ║                 ║      ║            │         ║    0 ║                      0-11 │       0 ║
    ║                 ║      ║            │         ║    1 ║                     12-23 │       0 ║
    ║                 ║    1 ║       cxi0 │       2 ║ ---- ║                     24-47 │       1 ║
    ║                 ║      ║            │         ║    0 ║                     24-35 │       1 ║
    ║                 ║      ║            │         ║    1 ║                     36-47 │       1 ║
    ║                 ║    2 ║       cxi0 │       2 ║ ---- ║                     48-71 │       2 ║
    ║                 ║      ║            │         ║    0 ║                     48-59 │       2 ║
    ║                 ║      ║            │         ║    1 ║                     60-71 │       2 ║
    ║                 ║    3 ║       cxi0 │       2 ║ ---- ║                     72-95 │       3 ║
    ║                 ║      ║            │         ║    0 ║                     72-83 │       3 ║
    ║                 ║      ║            │         ║    1 ║                     84-95 │       3 ║
    ║                 ║    4 ║       cxi1 │       6 ║ ---- ║                    96-119 │       4 ║
    ║                 ║      ║            │         ║    0 ║                    96-107 │       4 ║
    ║                 ║      ║            │         ║    1 ║                   108-119 │       4 ║
    ║                 ║    5 ║       cxi1 │       6 ║ ---- ║                   120-143 │       5 ║
    ║                 ║      ║            │         ║    0 ║                   120-131 │       5 ║
    ║                 ║      ║            │         ║    1 ║                   132-143 │       5 ║
    ║                 ║    6 ║       cxi1 │       6 ║ ---- ║                   144-167 │       6 ║
    ║                 ║      ║            │         ║    0 ║                   144-155 │       6 ║
    ║                 ║      ║            │         ║    1 ║                   156-167 │       6 ║
    ║                 ║    7 ║       cxi1 │       6 ║ ---- ║                   168-191 │       7 ║
    ║                 ║      ║            │         ║    0 ║                   168-179 │       7 ║
    ║                 ║      ║            │         ║    1 ║                   180-191 │       7 ║
    ╠═════════════════╬══════╬════════════╪═════════╬══════╬═══════════════════════════╪═════════╣
    ║      pinoak0052 ║      ║            │         ║      ║                           │         ║
    ║                 ║    8 ║       cxi0 │       2 ║ ---- ║                      0-23 │       0 ║
    ║                 ║      ║            │         ║    0 ║                      0-11 │       0 ║
    ║                 ║      ║            │         ║    1 ║                     12-23 │       0 ║
    ║                 ║    9 ║       cxi0 │       2 ║ ---- ║                     24-47 │       1 ║
    ║                 ║      ║            │         ║    0 ║                     24-35 │       1 ║
    ║                 ║      ║            │         ║    1 ║                     36-47 │       1 ║
    ║                 ║   10 ║       cxi0 │       2 ║ ---- ║                     48-71 │       2 ║
    ║                 ║      ║            │         ║    0 ║                     48-59 │       2 ║
    ║                 ║      ║            │         ║    1 ║                     60-71 │       2 ║
    ║                 ║   11 ║       cxi0 │       2 ║ ---- ║                     72-95 │       3 ║
    ║                 ║      ║            │         ║    0 ║                     72-83 │       3 ║
    ║                 ║      ║            │         ║    1 ║                     84-95 │       3 ║
    ║                 ║   12 ║       cxi1 │       6 ║ ---- ║                    96-119 │       4 ║
    ║                 ║      ║            │         ║    0 ║                    96-107 │       4 ║
    ║                 ║      ║            │         ║    1 ║                   108-119 │       4 ║
    ║                 ║   13 ║       cxi1 │       6 ║ ---- ║                   120-143 │       5 ║
    ║                 ║      ║            │         ║    0 ║                   120-131 │       5 ║
    ║                 ║      ║            │         ║    1 ║                   132-143 │       5 ║
    ║                 ║   14 ║       cxi1 │       6 ║ ---- ║                   144-167 │       6 ║
    ║                 ║      ║            │         ║    0 ║                   144-155 │       6 ║
    ║                 ║      ║            │         ║    1 ║                   156-167 │       6 ║
    ║                 ║   15 ║       cxi1 │       6 ║ ---- ║                   168-191 │       7 ║
    ║                 ║      ║            │         ║    0 ║                   168-179 │       7 ║
    ║                 ║      ║            │         ║    1 ║                   180-191 │       7 ║
    ╚═════════════════╩══════╩════════════╧═════════╩══════╩═══════════════════════════╧═════════╝


**ParryPeak (AMD APUs MI300A) node:**

    % srun -p parrypeak -N 1 -n 4 -c 24 --hint=nomultithread ./gpu-affinity.sh ./hpcat --no-banner
    ╔═════════════════╦══════╦═════════════════════════════════════════════╦═════════════════════════════════════╗
    ║            HOST ║  MPI ║                 ACCELERATORS                ║                          CPU        ║
    ║          (NODE) ║ RANK ║      ID │              PCIE ADDR. │    NUMA ║                   CORE ID │    NUMA ║
    ╠═════════════════╬══════╬═════════╪═════════════════════════╪═════════╬═══════════════════════════╪═════════╣
    ║      pinoak0001 ║      ║         │                         │         ║                           │         ║
    ║                 ║    0 ║       0 │                      00 │       0 ║                      0-23 │       0 ║
    ║                 ║    1 ║       1 │                      01 │       1 ║                     24-47 │       1 ║
    ║                 ║    2 ║       2 │                      02 │       2 ║                     48-71 │       2 ║
    ║                 ║    3 ║       3 │                      03 │       3 ║                     72-95 │       3 ║
    ╚═════════════════╩══════╩═════════╧═════════════════════════╧═════════╩═══════════════════════════╧═════════╝


**Exascale Compute Blade (Intel GPU Max 1550) node:**

    % srun -p ecb -N 1 -n 12 -c 8 --hint=nomultithread ./gpu-affinity.sh ./hpcat --no-banner
    ╔═════════════════╦══════╦═══════════════════════════════════════════════════╦════════════════════════════════════════════════╗
    ║            HOST ║  MPI ║                   ACCELERATORS                    ║                        CPU                     ║
    ║          (NODE) ║ RANK ║      ID │                    PCIE ADDR. │    NUMA ║         LOGICAL PROC │ PHYSICAL CORE │    NUMA ║
    ╠═════════════════╬══════╬═════════╪═══════════════════════════════╪═════════╬══════════════════════╪═══════════════╪═════════╣
    ║       nid000014 ║      ║         │                               │         ║                      │               │         ║
    ║                 ║    0 ║       0 │                          0:18 │       0 ║                  0-7 │           0-7 │       0 ║
    ║                 ║    1 ║       1 │                          0:18 │       0 ║                 8-15 │          8-15 │       0 ║
    ║                 ║    2 ║       2 │                          0:42 │       0 ║                16-23 │         16-23 │       0 ║
    ║                 ║    3 ║       3 │                          0:42 │       0 ║                24-31 │         24-31 │       0 ║
    ║                 ║    4 ║       4 │                          0:6c │       0 ║                32-39 │         32-39 │       0 ║
    ║                 ║    5 ║       5 │                          0:6c │       0 ║                40-47 │         40-47 │       0 ║
    ║                 ║    6 ║       6 │                          1:18 │       1 ║                52-59 │         52-59 │       1 ║
    ║                 ║    7 ║       7 │                          1:18 │       1 ║                60-67 │         60-67 │       1 ║
    ║                 ║    8 ║       8 │                          1:42 │       1 ║                68-75 │         68-75 │       1 ║
    ║                 ║    9 ║       9 │                          1:42 │       1 ║                76-83 │         76-83 │       1 ║
    ║                 ║   10 ║      10 │                          1:6c │       1 ║                84-91 │         84-91 │       1 ║
    ║                 ║   11 ║      11 │                          1:6c │       1 ║                92-99 │         92-99 │       1 ║
    ╚═════════════════╩══════╩═════════╧═══════════════════════════════╧═════════╩══════════════════════╧═══════════════╧═════════╝

