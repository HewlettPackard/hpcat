INCLUDE(ExternalProject)

EXTERNALPROJECT_ADD(hwloc
    SOURCE_DIR ${CMAKE_CURRENT_SOURCE_DIR}/../submodules/hwloc
    CONFIGURE_COMMAND ${CMAKE_CURRENT_SOURCE_DIR}/../submodules/hwloc/autogen.sh COMMAND ${CMAKE_CURRENT_SOURCE_DIR}/../submodules/hwloc/configure --disable-opencl --disable-cuda --disable-nvml --disable-rsmi --disable-levelzero --disable-gl --disable-libudev --enable-static --disable-shared --disable-pci --disable-io --disable-cairo --disable-libxml2 --with-pic --prefix=<INSTALL_DIR>
    BUILD_COMMAND ${MAKE}
)

EXTERNALPROJECT_GET_PROPERTY(hwloc install_dir)
SET(HWLOC_INSTALL_PATH "${install_dir}" CACHE INTERNAL "")
