Thare many mode to run application, the build step is dependent the mode the app run:

- run with umd_mode = 0, which can run ptx sim or opu isasim
. build libcuda
> cd design
> source setup_env.sh
> cd libgem5cuda; make
> cd libcuda; make

- run with umd_mode = 1, which app run with gem5
. update design/gpgpu/gpgpu-sim/gem5.env
. build gem5gpu
> cd design
> source setup_env.sh
> build.sh

- run app outside gem5
. build libgem5
> cd design
> ./build_libgem5.sh
and copy $GEM5_ROOT/build/X86_VI_hammer/libgem5_$VARIANT.so to $GEM5_ROOT/cosim

- run gem5 mixed with systemc
. build sc cosim
> cd design/cosim
> scons


## run howto

in run directory bchange gpgpusim.config for different umd mode

1. isasim or libcuda ptx simulation mode
-gpgpu_umd_mode 0

2. run app in gem5
-gpgpu_umd_mode 1

at begining, run design/opu/oputest/cuda_samples/smoke test to verify it works
currently there are tests in smoke:
- vectorCopy
- vectorAdd
- vectorSmem
- matrixMul
- simpleTensorCoreGemm
- shmemTensorCoreGemm


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

note: you can use design/.gdbini, which run gdb at design directory

## m5 cpu/gpu api syscall

the m5 op is define in util/m5/m5op_x85.S, which define the  x86 macro instruction.

代码中插入, m5op::m5_gpu(__gpusysno, call_params)
来执行

插入后的代码，会被src/arch/x86/isa/decoder/two_byte_opcodes.isa
解析后会调用相应的PseudoInst

src/sim/PseudoInst.cc 定义了通过PseduoInst调用的系统调用，
比如m5_gpu的系统调用
PseudoInst::gpu

其中调用的函数gpgpu_funcs

## mmu

since app run outside gem5, it use AppThreadContext as faked context to run gem5 cuda api 
we use setting
- manageGPUMemory is true for libgem5cuda
- accessHostPage is false
	we use hdp to read/write app allocated memory data
	and hack ShaderTLB::beginTranslating to use paddr same as vaddr

## config.ini setup procedure

### se_fusion.py
  - GPUConfig.craeteGPU (design/cosim/configs/GPUConfigs.py) to setup gpu
  - Ruby.create_system (
		config/Ruby
		design/cosim/configs/gpu_protocal/VI_hammer_fusion.py
		design/cosim/configs/gpu_protocal/VI_hammer.py
	  )
  - GPUConfig.connectGPUPorts

