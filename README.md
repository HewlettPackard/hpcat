HPC Affinity Tracker (HPCAT)
============================

This application is designed to display NUMA and CPU affinities within the
context of HPC applications. It provides reports on MPI tasks, OpenMP
(automatically enabled if OMP_NUM_THREADS is set), accelerators (automatically
enabled if GPUs are allocated via Slurm), and NICs (Cray MPICH only, starting
from 2 nodes). The output format is a human-readable, condensed table, but YAML
is also available as an option.

The application uses dynamic linking modules to retrieve information about
accelerators, allowing the same binary to be used across different partitions,
whether or not accelerators are present.


Dependencies
------------

* **NVIDIA NVML** (Optional, for NVIDIA GPUs)
* **AMD ROCm** (Optional, for AMD GPUs)
* **hwloc-devel**

If hwloc-devel is not installed, it can be re-compiler that way:

    wget https://download.open-mpi.org/release/hwloc/v2.11/hwloc-2.11.2.tar.gz
    tar xzf hwloc-2.11.2.tar.gz
    cd hwloc-2.11.2/
    ./configure --prefix=$HOME/TOOLS/hwloc
    make
    make install
    export C_INCLUDE_PATH=$HOME/TOOLS/hwloc/include:$C_INCLUDE_PATH
    export LIBRARY_PATH=$HOME/TOOLS/hwloc/lib:$LIBRARY_PATH


How to build HPCAT
------------------

Main program (no accelerator):

    % module purge
    % module load PrgEnv-cray
    % make

Dynamic library module for NVIDIA GPUs:

    % module purge
    % module load cuda
    % make nvidia

Dynamic library module for AMD GPUs:

    % module purge
    % module load rocm
    % make amd


Note: the dynamic library modules should be stored in the same directory as the hpcat binary.


How to run HPCAT
----------------

    % hpcat [ARGS...]

Arguments are :

        --disable-accel        Disable display of GPU affinities
        --disable-nic          Disable display of Network Interface affinities
        --disable-omp          Disable display of OpenMP affinities
        --enable-accel         Enable display of GPU affinities
        --enable-omp           Enable display of OpenMP affinities
        --no-banner            Do not display header and footer banners
    -y, --yaml                 YAML output
    -?, --help                 Give this help list
        --usage                Give a short usage message
    -V, --version              Print program version


Examples
--------

**Bardpeak:**

    % srun -p bardpeak -N2 --tasks-per-node=8 --cpu-bind=mask_cpu:0xfe000000000000,0xfe00000000000000,0xfe0000,0xfe000000,\
    0xfe,0xfe00,0xfe00000000,0xfe0000000000 -t 00:01:00 --gpus-per-task=1 --hint=nomultithread ./hpcat -o
    MPI VERSION    : CRAY MPICH version 8.1.28.15 (ANL base 3.4a2)
    ╔════════════════════════════════════════════════════════════════════════════════════════════════════════════════════════════════╗
    ║                                                       HPC Affinity Tracker                                                v0.1 ║
    ╠═════════════════╦══════╦══════════════════════╦═══════════════════════════════════╦══════╦═════════════════════════════════════╣
    ║            HOST ║  MPI ║         NETWORK      ║                   ACCELERATORS    ║  OMP ║                          CPU        ║
    ║          (NODE) ║ RANK ║  INTERFACE │    NUMA ║               PCIE ADDR │    NUMA ║   ID ║                   CORE ID │    NUMA ║
    ╠═════════════════╬══════╬════════════╪═════════╬═════════════════════════╪═════════╬══════╬═══════════════════════════╪═════════╣
    ║           g1239 ║      ║            │         ║                         │         ║      ║                           │         ║
    ║                 ║    0 ║       cxi0 │       3 ║                      c1 │       3 ║ ---- ║                     49-55 │       3 ║
    ║                 ║    1 ║       cxi0 │       3 ║                      c6 │       3 ║ ---- ║                     57-63 │       3 ║
    ║                 ║    2 ║       cxi1 │       1 ║                      c9 │       1 ║ ---- ║                     17-23 │       1 ║
    ║                 ║    3 ║       cxi1 │       1 ║                      ce │       1 ║ ---- ║                     25-31 │       1 ║
    ║                 ║    4 ║       cxi2 │       0 ║                      d1 │       0 ║ ---- ║                       1-7 │       0 ║
    ║                 ║    5 ║       cxi2 │       0 ║                      d6 │       0 ║ ---- ║                      9-15 │       0 ║
    ║                 ║    6 ║       cxi3 │       2 ║                      d9 │       2 ║ ---- ║                     33-39 │       2 ║
    ║                 ║    7 ║       cxi3 │       2 ║                      de │       2 ║ ---- ║                     41-47 │       2 ║
    ╠═════════════════╬══════╬════════════╪═════════╬═════════════════════════╪═════════╬══════╬═══════════════════════════╪═════════╣
    ║           g1240 ║      ║            │         ║                         │         ║      ║                           │         ║
    ║                 ║    8 ║       cxi0 │       3 ║                      c1 │       3 ║ ---- ║                     49-55 │       3 ║
    ║                 ║    9 ║       cxi0 │       3 ║                      c6 │       3 ║ ---- ║                     57-63 │       3 ║
    ║                 ║   10 ║       cxi1 │       1 ║                      c9 │       1 ║ ---- ║                     17-23 │       1 ║
    ║                 ║   11 ║       cxi1 │       1 ║                      ce │       1 ║ ---- ║                     25-31 │       1 ║
    ║                 ║   12 ║       cxi2 │       0 ║                      d1 │       0 ║ ---- ║                       1-7 │       0 ║
    ║                 ║   13 ║       cxi2 │       0 ║                      d6 │       0 ║ ---- ║                      9-15 │       0 ║
    ║                 ║   14 ║       cxi3 │       2 ║                      d9 │       2 ║ ---- ║                     33-39 │       2 ║
    ║                 ║   15 ║       cxi3 │       2 ║                      de │       2 ║ ---- ║                     41-47 │       2 ║
    ╠═════════════════╬══════╬════════════╪═════════╬═════════════════════════╪═════════╬══════╬═══════════════════════════╪═════════╣
    ║            HOST ║  MPI ║         NETWORK      ║                   ACCELERATORS    ║  OMP ║                          CPU        ║
    ║          (NODE) ║ RANK ║  INTERFACE │    NUMA ║               PCIE ADDR │    NUMA ║   ID ║                   CORE ID │    NUMA ║
    ╠═════════════════╬══════╬════════════╧═════════╩═════════════════════════╧═════════╩══════╩═══════════════════════════╧═════════╝
    ║     TOTAL:    2 ║   16 ║
    ╚═════════════════╩══════╝


**Antero:**

    % OMP_NUM_THREADS=2 srun -p antero -N2 --tasks-per-node=8 -c 24 --hint=nomultithread ./hpcat
    MPI VERSION    : CRAY MPICH version 8.1.30.8 (ANL base 3.4a2)
    ╔════════════════════════════════════════════════════════════════════════════════════════════════════════════════════════════════╗
    ║                                                       HPC Affinity Tracker                                                v0.1 ║
    ╠═════════════════╦══════╦══════════════════════╦═══════════════════════════════════╦══════╦═════════════════════════════════════╣
    ║            HOST ║  MPI ║         NETWORK      ║                   ACCELERATORS    ║  OMP ║                          CPU        ║
    ║          (NODE) ║ RANK ║  INTERFACE │    NUMA ║               PCIE ADDR │    NUMA ║   ID ║                   CORE ID │    NUMA ║
    ╠═════════════════╬══════╬════════════╪═════════╬═════════════════════════╪═════════╬══════╬═══════════════════════════╪═════════╣
    ║      pinoak0051 ║      ║            │         ║                         │         ║      ║                           │         ║
    ║                 ║    0 ║       cxi0 │       2 ║                         │         ║ ---- ║                      0-23 │       0 ║
    ║                 ║      ║            │         ║                         │         ║    0 ║                      0-11 │       0 ║
    ║                 ║      ║            │         ║                         │         ║    1 ║                     12-23 │       0 ║
    ║                 ║    1 ║       cxi0 │       2 ║                         │         ║ ---- ║                     24-47 │       1 ║
    ║                 ║      ║            │         ║                         │         ║    0 ║                     24-35 │       1 ║
    ║                 ║      ║            │         ║                         │         ║    1 ║                     36-47 │       1 ║
    ║                 ║    2 ║       cxi0 │       2 ║                         │         ║ ---- ║                     48-71 │       2 ║
    ║                 ║      ║            │         ║                         │         ║    0 ║                     48-59 │       2 ║
    ║                 ║      ║            │         ║                         │         ║    1 ║                     60-71 │       2 ║
    ║                 ║    3 ║       cxi0 │       2 ║                         │         ║ ---- ║                     72-95 │       3 ║
    ║                 ║      ║            │         ║                         │         ║    0 ║                     72-83 │       3 ║
    ║                 ║      ║            │         ║                         │         ║    1 ║                     84-95 │       3 ║
    ║                 ║    4 ║       cxi1 │       6 ║                         │         ║ ---- ║                    96-119 │       4 ║
    ║                 ║      ║            │         ║                         │         ║    0 ║                    96-107 │       4 ║
    ║                 ║      ║            │         ║                         │         ║    1 ║                   108-119 │       4 ║
    ║                 ║    5 ║       cxi1 │       6 ║                         │         ║ ---- ║                   120-143 │       5 ║
    ║                 ║      ║            │         ║                         │         ║    0 ║                   120-131 │       5 ║
    ║                 ║      ║            │         ║                         │         ║    1 ║                   132-143 │       5 ║
    ║                 ║    6 ║       cxi1 │       6 ║                         │         ║ ---- ║                   144-167 │       6 ║
    ║                 ║      ║            │         ║                         │         ║    0 ║                   144-155 │       6 ║
    ║                 ║      ║            │         ║                         │         ║    1 ║                   156-167 │       6 ║
    ║                 ║    7 ║       cxi1 │       6 ║                         │         ║ ---- ║                   168-191 │       7 ║
    ║                 ║      ║            │         ║                         │         ║    0 ║                   168-179 │       7 ║
    ║                 ║      ║            │         ║                         │         ║    1 ║                   180-191 │       7 ║
    ╠═════════════════╬══════╬════════════╪═════════╬═════════════════════════╪═════════╬══════╬═══════════════════════════╪═════════╣
    ║      pinoak0052 ║      ║            │         ║                         │         ║      ║                           │         ║
    ║                 ║    8 ║       cxi0 │       2 ║                         │         ║ ---- ║                      0-23 │       0 ║
    ║                 ║      ║            │         ║                         │         ║    0 ║                      0-11 │       0 ║
    ║                 ║      ║            │         ║                         │         ║    1 ║                     12-23 │       0 ║
    ║                 ║    9 ║       cxi0 │       2 ║                         │         ║ ---- ║                     24-47 │       1 ║
    ║                 ║      ║            │         ║                         │         ║    0 ║                     24-35 │       1 ║
    ║                 ║      ║            │         ║                         │         ║    1 ║                     36-47 │       1 ║
    ║                 ║   10 ║       cxi0 │       2 ║                         │         ║ ---- ║                     48-71 │       2 ║
    ║                 ║      ║            │         ║                         │         ║    0 ║                     48-59 │       2 ║
    ║                 ║      ║            │         ║                         │         ║    1 ║                     60-71 │       2 ║
    ║                 ║   11 ║       cxi0 │       2 ║                         │         ║ ---- ║                     72-95 │       3 ║
    ║                 ║      ║            │         ║                         │         ║    0 ║                     72-83 │       3 ║
    ║                 ║      ║            │         ║                         │         ║    1 ║                     84-95 │       3 ║
    ║                 ║   12 ║       cxi1 │       6 ║                         │         ║ ---- ║                    96-119 │       4 ║
    ║                 ║      ║            │         ║                         │         ║    0 ║                    96-107 │       4 ║
    ║                 ║      ║            │         ║                         │         ║    1 ║                   108-119 │       4 ║
    ║                 ║   13 ║       cxi1 │       6 ║                         │         ║ ---- ║                   120-143 │       5 ║
    ║                 ║      ║            │         ║                         │         ║    0 ║                   120-131 │       5 ║
    ║                 ║      ║            │         ║                         │         ║    1 ║                   132-143 │       5 ║
    ║                 ║   14 ║       cxi1 │       6 ║                         │         ║ ---- ║                   144-167 │       6 ║
    ║                 ║      ║            │         ║                         │         ║    0 ║                   144-155 │       6 ║
    ║                 ║      ║            │         ║                         │         ║    1 ║                   156-167 │       6 ║
    ║                 ║   15 ║       cxi1 │       6 ║                         │         ║ ---- ║                   168-191 │       7 ║
    ║                 ║      ║            │         ║                         │         ║    0 ║                   168-179 │       7 ║
    ║                 ║      ║            │         ║                         │         ║    1 ║                   180-191 │       7 ║
    ╠═════════════════╬══════╬════════════╪═════════╬═════════════════════════╪═════════╬══════╬═══════════════════════════╪═════════╣
    ║            HOST ║  MPI ║         NETWORK      ║                   ACCELERATORS    ║  OMP ║                          CPU        ║
    ║          (NODE) ║ RANK ║  INTERFACE │    NUMA ║               PCIE ADDR │    NUMA ║   ID ║                   CORE ID │    NUMA ║
    ╠═════════════════╬══════╬════════════╧═════════╩═════════════════════════╧═════════╩══════╩═══════════════════════════╧═════════╝
    ║     TOTAL:    2 ║   16 ║
    ╚═════════════════╩══════╝


**ParryPeak:**

    % srun -p parrypeak  -N 2 -n 8 --gpus-per-task=1 -c 24 -t 00:01:00 --hint=nomultithread ./hpcat -o
    MPI VERSION    : CRAY MPICH version 8.1.30.1 (ANL base 3.4a2)
    ╔════════════════════════════════════════════════════════════════════════════════════════════════════════════════════════════════╗
    ║                                                       HPC Affinity Tracker                                                v0.1 ║
    ╠═════════════════╦══════╦══════════════════════╦═══════════════════════════════════╦══════╦═════════════════════════════════════╣
    ║            HOST ║  MPI ║         NETWORK      ║                   ACCELERATORS    ║  OMP ║                          CPU        ║
    ║          (NODE) ║ RANK ║  INTERFACE │    NUMA ║               PCIE ADDR │    NUMA ║   ID ║                   CORE ID │    NUMA ║
    ╠═════════════════╬══════╬════════════╪═════════╬═════════════════════════╪═════════╬══════╬═══════════════════════════╪═════════╣
    ║           a1001 ║      ║            │         ║                         │         ║      ║                           │         ║
    ║                 ║    0 ║       cxi0 │       0 ║                      00 │       0 ║ ---- ║                      0-23 │       0 ║
    ║                 ║    1 ║       cxi1 │       1 ║                      01 │       1 ║ ---- ║                     24-47 │       1 ║
    ║                 ║    2 ║       cxi2 │       2 ║                      02 │       2 ║ ---- ║                     48-71 │       2 ║
    ║                 ║    3 ║       cxi3 │       3 ║                      03 │       3 ║ ---- ║                     72-95 │       3 ║
    ╠═════════════════╬══════╬════════════╪═════════╬═════════════════════════╪═════════╬══════╬═══════════════════════════╪═════════╣
    ║           a1003 ║      ║            │         ║                         │         ║      ║                           │         ║
    ║                 ║    4 ║       cxi0 │       0 ║                      00 │       0 ║ ---- ║                      0-23 │       0 ║
    ║                 ║    5 ║       cxi1 │       1 ║                      01 │       1 ║ ---- ║                     24-47 │       1 ║
    ║                 ║    6 ║       cxi2 │       2 ║                      02 │       2 ║ ---- ║                     48-71 │       2 ║
    ║                 ║    7 ║       cxi3 │       3 ║                      03 │       3 ║ ---- ║                     72-95 │       3 ║
    ╠═════════════════╬══════╬════════════╪═════════╬═════════════════════════╪═════════╬══════╬═══════════════════════════╪═════════╣
    ║            HOST ║  MPI ║         NETWORK      ║                   ACCELERATORS    ║  OMP ║                          CPU        ║
    ║          (NODE) ║ RANK ║  INTERFACE │    NUMA ║               PCIE ADDR │    NUMA ║   ID ║                   CORE ID │    NUMA ║
    ╠═════════════════╬══════╬════════════╧═════════╩═════════════════════════╧═════════╩══════╩═══════════════════════════╧═════════╝
    ║     TOTAL:    2 ║    8 ║
    ╚═════════════════╩══════╝


Future Work
-----------

- [ ] If any issue raised with line ordering in the output, make first MPI rank print all lines (no issue so far)
- [ ] Support Intel's Ponte Vecchio GPUs
- [ ] Support (if possible) NIC affinities with other MPI distributions
- [ ] Find a better way to autodetect GPUs (GPU columns can still be displayed using the appropriate option). Some sites are installing GPU libraries on CPU nodes (checking presence of those libraries is not an option). Checking SLURM environment variables is not enough as some clusters are not using GRES or are relying on another WLM
