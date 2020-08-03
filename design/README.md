
1. build libcuda
> cd design/gpgpu/gpgpu-sim
> source setup_env.sh
> cd libgem5cuda; make
> cd libcuda; make

2. update design/gpgpu/gpgpu-sim/gem5.env

3. build gem5gpu
> source design/setup_env.sh
> design/build.sh


## debug howto

the app run in gem5 is debugged by remote gdb

1. use -w option with gem5.debug
2. run command in remote gdb
> set remote z-packet on
> set debug serial 1
> #handle SIGPIPE nostop print
> #handle SIGIO nostop noprint
> #handle SIGTRAP nostop noprint
> file ....
> target remote localhost:7000


