# Changelog

All notable changes to this project will be documented in this file.
The format is based on [Keep a Changelog](https://keepachangelog.com/en/1.1.0/),

## [v0.8] - 2025-05-28

### Added

- Introduced a color modification feature for dark and light terminal themes.
- Added a module file for easy modification of environment variables and tool execution.
- Included a man page for improved user guidance. Acknowledgments to Kurt Lust for the initial version.

### Changed

- Disabled automatic color detection; no color will be used by default.
- Enhanced documentation by revising the usage and build sections for better clarity.
- Refactored installation paths to separate binary and library subdirectories.

### Fixed

- Simplified the code by retrieving MAC addresses for the Slingshot Dragonfly group without using sockets.


## [v0.7] - 2025-04-29

### Added

- Support for Slingshot Dragonfly group ID detection.
- Scalability plot from 2 to 256 nodes with 8 MPI tasks per node (execution time < 3 seconds).
- Forwarding the CC variable from the configure wrapper to CMake for easier compiler switching.

### Changed

- Accelerated topology detection with hwloc by limiting discovery to one rank per node and sharing the results with local ranks.
- Reduced colored output overhead with libfort by using OMP lines as the default color.

### Fixed

- Addressed compilation and linking issues with compilers other than Cray Compilers.


## [v0.6] - 2025-04-09

### Added

- Support for Intel GPUs via oneAPI Level Zero.
- Colored tabular output.

### Changed

- Switched to CMake for the build system and added a configuration wrapper.
- Replaced internal table printing with libfort.
- Now building the hwloc submodule as part of HPCAT.
- Replaced pipe2 with pipe (no flags were used).
- Always display the accelerator PCIe domain and added brackets around each PCIe address.

### Fixed

- Resolved inconsistent output related to OpenMP thread affinities.
- Resolved impact of CUDA_VISIBLE_DEVICES being ignored by NVML.


## [v0.5] - 2025-03-18

### Added

- Column to see if hardware threads (CPU IDs) are mapped to the same CPU core.
- This CHANGELOG.md file.

### Changed

- First rank gathers all data prints the output.
- Disable OpenMP output by default if only one thread is used.


## [v0.4] - 2024-12-17

### Added

- Column to seen defined ROCR|CUDA_VISIBLE_DEVICES environment variables and associated GPU IDs.
- Verbose option. Currently used to verify that all steps and dynamic modules are properly loaded.

### Changed

- Rely on the search path to check libnvidia-ml and libamdhip64 availabilities.
- Auto accelerator disabling: now counts all visible accelerators (allreduce) and disables the column if zero.

[v0.8]: https://github.com/HewlettPackard/hpcat/compare/v0.7...v0.8
[v0.7]: https://github.com/HewlettPackard/hpcat/compare/v0.6...v0.7
[v0.6]: https://github.com/HewlettPackard/hpcat/compare/v0.5...v0.6
[v0.5]: https://github.com/HewlettPackard/hpcat/compare/v0.4...v0.5
[v0.4]: https://github.com/HewlettPackard/hpcat/compare/v0.3...v0.4

