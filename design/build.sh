#rm build/variables.global
DIR="$( cd "$( dirname "${BASH_SOURCE[0]}" )" && pwd )"

# cd gem5
#scons build/X86_VI_hammer_GPU/gem5.opt --default=../../../gem5-gpu/build_opts/X86_VI_hammer_GPU EXTRAS=../gem5-gpu/src:../gpgpu-sim/

#scons build/X86_VI_hammer_GPU/gem5.opt --verbose


#scons build/PPU/gem5.debug --verbose -j6

#scons build/RISCV/gem5.debug --verbose -j6
#scons build/RISCV/gem5.debug


#scons build/X86/gem5.debug --verbose BUILD_GPU=yes  PROTOCOL=GPU_VIPER_Baseline -j6
#scons build/X86/gem5.debug --verbose -j6


#scons build/GPU_X86/gem5.debug --verbose -j6
#scons build/GPU_X86/gem5.debug --verbose BUILD_GPU=yes -j6


#scons build/HSAIL_X86/gem5.opt --verbose -j6
#scons build/HSAIL_X86/gem5.opt --verbose BUILD_GPU=yes  PROTOCOL=GPU_VIPER_Baseline -j6



#scons build/NULL/gem5.debug --verbose -j6 PROTOCOL=Garnet_standalone
#scons build/NULL_MOESI_hammer/gem5.debug --verbose -j8

#scons build/MSI/gem5.debug --verbose -j6 PROTOCOL=MSI

#############################
# for learning_gem5 MSI
#scons build/X86_MSI/gem5.debug -j8 --verbose PROTOCOL=MSI
#scons build/X86_MESI_Two_Level/gem5.debug -j8 --verbose
#exit

#scons build/MSI/gem5.opt --verbose -j6



#############################
# for GPGPU
#rm build/variables/X86_VI_hammer
#scons build/X86_VI_hammer/gem5.opt --verbose -j8 BUILD_PPU=yes --default=$DIR/../design/gpgpu/build_opts/X86_VI_hammer EXTRAS=design/gpgpu
scons -j8 build/X86_VI_hammer/gem5.$GEM5_VARIANT --verbose -j8 --default=$DIR/../design/gpgpu/build_opts/X86_VI_hammer EXTRAS=design/gpgpu

#scons --with-cxx-config --without-python --without-tcmalloc build/X86_VI_hammer/libgem5_debug.so --verbose BUILD_PPU=yes --default=../design/gpgpu/build_opts/X86_VI_hammer EXTRAS=design/gpgpu
