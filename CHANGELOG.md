# Changelog

All notable changes to this project will be documented in this file.
The format is based on [Keep a Changelog](https://keepachangelog.com/en/1.1.0/),

## [0.5] - 2025-03-18

### Added

- Column to see if hardware threads (CPU IDs) are mapped to the same CPU core.
- This CHANGELOG.md file.

### Changed

- First rank gathers all data prints the output
- Disable OpenMP output by default if only one thread is used


## [0.4] - 2024-12-17

### Added

- Column to seen defined ROCR|CUDA_VISIBLE_DEVICES environment variables and associated GPU IDs.
- Verbose option. Currently used to verify that all steps and dynamic modules are properly loaded.

### Changed

- Rely on the search path to check libnvidia-ml and libamdhip64 availabilities.
- Auto accelerator disabling: now counts all visible accelerators (allreduce) and disables the column if zero.

[0.5]: https://github.com/HewlettPackard/hpcat/compare/v0.4...v0.5
[0.4]: https://github.com/HewlettPackard/hpcat/compare/v0.3...v0.4

