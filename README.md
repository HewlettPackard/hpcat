HPC Affinity Tracker (HPCAT)
============================

This application is designed to display NUMA, CPU core and GPU affinities within
the context of HPC applications. It provides reports on MPI tasks, OpenMP
(automatically enabled if `OMP_NUM_THREADS` is set), accelerators (automatically
enabled if GPUs are detected), and NICs (Cray MPICH only, starting from 2 nodes).
The output format is a human-readable, condensed table, but YAML is also available
as an option.

The application uses dynamic linking modules to retrieve information about
accelerators, allowing the same binary to be used across different partitions,
whether or not accelerators are present.


Dependencies
------------

* **NVIDIA NVML** (Optional, for NVIDIA GPUs)
* **AMD ROCm** (Optional, for AMD GPUs)
* **hwloc-devel**
* **MPI**

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

**HPE Cray Programming Environment (CPE)**

With cray environment:

    module load PrgEnv-cray
    module load rocm # Optional
    module load cuda # Optional
    module unload craype
    make

With GNU enironment:

    module load PrgEnv-gnu
    module load rocm # Optional
    module load cuda # Optional
    module unload craype
    make


**Without CPE**

Ensure that mpicc is available otherwise set the MPICC environment variable.
To enable the AMD plugin, ensure the `HIP_PATH` environment variable is set
To enabble the NVIDIA plugin, ensure the `CUDA_HOME` environment variable is set

    make


*Note: the dynamic library modules should be stored in the same directory as the hpcat binary.*


How to run HPCAT
----------------

    % hpcat [ARGS...]

Arguments are :

        --disable-accel        Disable display of GPU affinities
        --disable-nic          Disable display of Network Interface affinities
        --disable-omp          Disable display of OpenMP affinities
        --enable-omp           Enable display of OpenMP affinities
        --no-banner            Do not display header and footer banners
    -v, --verbose              Make the operation more talkative
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


**ParryPeak (AMD APUs MI300A) nodes:**

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


Future Work
-----------

- [ ] If any issue raised with line ordering in the output, make first MPI rank print all lines (no issue so far)
- [ ] Support Intel's Ponte Vecchio GPUs
- [ ] Support (if possible) NIC affinities with other MPI distributions
