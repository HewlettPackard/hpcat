ADD_SUBDIRECTORY(amd)
ADD_SUBDIRECTORY(intel)
ADD_SUBDIRECTORY(nvidia)

CONFIGURE_FILE(
    "${CMAKE_CURRENT_SOURCE_DIR}/version.h.in"
    "${CMAKE_CURRENT_BINARY_DIR}/version.h"
    @ONLY
)

FIND_PACKAGE(MPI REQUIRED)

# Enable OpenMP
SET(CMAKE_C_FLAGS "${CMAKE_C_FLAGS} -fopenmp")

# Using libfort without string literal
SET(CMAKE_C_FLAGS "${CMAKE_C_FLAGS} -g -Wno-format-security")

INCLUDE_DIRECTORIES(SYSTEM ${MPI_INCLUDE_PATH} ${HWLOC_INSTALL_PATH}/include ${CMAKE_CURRENT_BINARY_DIR} ${CMAKE_CURRENT_SOURCE_DIR}/../submodules/libfort/lib)
ADD_EXECUTABLE(hpcat hpcat.c output.c settings.c hint.c ${CMAKE_CURRENT_SOURCE_DIR}/../submodules/libfort/lib/fort.c)
ADD_DEPENDENCIES(hpcat hwloc)

TARGET_LINK_LIBRARIES(hpcat dl ${MPI_C_LIBRARIES} ${HWLOC_INSTALL_PATH}/lib/libhwloc.a)

INSTALL(TARGETS hpcat DESTINATION ${CMAKE_INSTALL_PREFIX}/bin)

# Create the Module file content
FILE(WRITE "${CMAKE_CURRENT_BINARY_DIR}/${PROJECT_VERSION}"
        "#%Module1.0"
        "module-whatis \"HPCAT (version ${PROJECT_VERSION})\"\n"
        "proc ModulesHelp { } {\n"
        "    puts stderr \"Loads HPCAT v${PROJECT_VERSION}\"\n"
        "}\n"
        "set HPCAT_DIR [file normalize [file dirname [file normalize \${ModulesCurrentModulefile})]]/../../..]\n"
        "setenv HPCAT_VERSION \"${PROJECT_VERSION}\"\n"
        "prepend-path MANPATH \"$HPCAT_DIR/share/man\"\n"
        "prepend-path PATH \"$HPCAT_DIR/bin\"\n")

# Install Module file
INSTALL(FILES "${CMAKE_CURRENT_BINARY_DIR}/${PROJECT_VERSION}" DESTINATION "${CMAKE_INSTALL_PREFIX}/share/modulefiles/hpcat")
