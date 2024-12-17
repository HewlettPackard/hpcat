all:
	cc -O3 -L. -I. -lhwloc -fopenmp hpcat.c output.c settings.c -o hpcat -ldl

debug:
	cc -Wall -g -L. -I. -lhwloc -fopenmp hpcat.c output.c settings.c -o hpcat -ldl

amd:
	cc -O3 -L. -I. -I/opt/rocm/include -D__HIP_PLATFORM_AMD__ -Wl,-rpath,'/opt/rocm/lib' accel_hip.c -lhwloc -shared -fPIC -ldl -o hpcathip.so

nvidia:
	cc -O3 -L. -I. accel_nvml.c -lhwloc -L${CUDA_HOME}/lib64 -Wl,-rpath,'${CUDA_HOME}/lib64' -lnvidia-ml -shared -fPIC -ldl -o hpcatnvml.so

clean:
	@rm -f hpcat

distclean: clean
	@rm -f hpcathip.so
	@rm -f hpcatnvml.so
