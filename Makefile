MPICC ?= mpicc
CC = gcc

ifneq ($(shell which hipconfig 2>/dev/null),)
  HIP_PATH ?= $(shell hipconfig --path)
endif

TARGETS := main

ifdef HIP_PATH
  TARGETS += amd
endif

ifdef CUDA_HOME
  TARGETS += nvidia
endif

all: $(TARGETS)

main:
	${MPICC} -O3 -L. -I. -lhwloc -fopenmp hpcat.c output.c settings.c -o hpcat -ldl

debug:
	${MPICC} -Wall -g -L. -I. -lhwloc -fopenmp hpcat.c output.c settings.c -o hpcat -ldl

amd:
	${CC} -O3 -L. -I. -L${HIP_PATH}/lib -I${HIP_PATH}/include -D__HIP_PLATFORM_AMD__ -Wl,-rpath,'${HIP_PATH}/lib' -lamdhip64 -lhwloc -shared -fPIC -ldl accel_hip.c -o hpcathip.so

nvidia:
	${CC} -O3 -L. -I. -L${CUDA_HOME}/lib64 -L${CUDA_HOME}/lib64/stubs -L${CUDA_HOME}/include -Wl,-rpath,'${CUDA_HOME}/lib64' -Wl,-rpath,'${CUDA_HOME}/lib64/stubs' -lnvidia-ml -lhwloc -shared -fPIC -ldl accel_nvml.c -o hpcatnvml.so

clean:
	@rm -f hpcat
	@rm -f hpcathip.so
	@rm -f hpcatnvml.so
