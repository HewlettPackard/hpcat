ADD_SUBDIRECTORY(amd)
ADD_SUBDIRECTORY(intel)
ADD_SUBDIRECTORY(nvidia)

FIND_PACKAGE(MPI REQUIRED)

# Enable OpenMP
SET(CMAKE_C_FLAGS "${CMAKE_C_FLAGS} -fopenmp")

# Using libfort without string literal
SET(CMAKE_C_FLAGS "${CMAKE_C_FLAGS} -g -Wno-format-security")

INCLUDE_DIRECTORIES(SYSTEM ${MPI_INCLUDE_PATH} ${HWLOC_INSTALL_PATH}/include ${CMAKE_CURRENT_SOURCE_DIR}/../submodules/libfort/lib)
ADD_EXECUTABLE(hpcat hpcat.c output.c settings.c ${CMAKE_CURRENT_SOURCE_DIR}/../submodules/libfort/lib/fort.c)
ADD_DEPENDENCIES(hpcat hwloc)

TARGET_LINK_LIBRARIES(hpcat dl ${MPI_C_LIBRARIES} ${HWLOC_INSTALL_PATH}/lib/libhwloc.a)

INSTALL(TARGETS hpcat DESTINATION ${CMAKE_INSTALL_PREFIX})
