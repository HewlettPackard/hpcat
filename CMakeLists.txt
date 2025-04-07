CMAKE_MINIMUM_REQUIRED(VERSION 3.0)

PROJECT(hpcat)

OPTION(DEBUG "Debug mode." OFF)

IF(DEBUG)
    SET(CMAKE_C_FLAGS "-O0 -g -fsanitize=address -fno-omit-frame-pointer -fopenmp")
ENDIF(DEBUG)

ADD_SUBDIRECTORY(submodules)
ADD_SUBDIRECTORY(src)
