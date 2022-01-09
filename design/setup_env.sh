DIR="$( cd "$( dirname "${BASH_SOURCE[0]}" )" && cd .. && pwd )"
# is is used to build benchmark
#
if [ x"$PPFLOW_ROOT" == "x" ];
then
	PPFLOW_ROOT=$DIR/../../mixlang
fi

export GEM5_ROOT=$DIR
#export GEM5_VARIANT=debug
export GEM5_VARIANT=opt
export GEM5_ARCH=X86_VI_hammer
#export CUDAHOME=/usr/local/cuda/
#export CUDAHOME=$DIR/cuda3.2

NVCC_PATH=`which nvcc`
if [ x$NVCC_PATH == "x" ]; then
	echo "Can't find nvcc "
	return 1
fi

export CUDAHOME=`echo $NVCC_PATH | sed -re 's#/bin/nvcc##'`
echo "find cuda path $CUDAHOME"

CUDA_VERSION=`nvcc --version |grep release | sed 's#.*release ##' | cut -d',' -f1 |sed 's#\.##'`


export PATH="$CUDAHOME/bin:$PATH"
export CUDA_INSTALL_PATH=$CUDAHOME
export CUDA_PATH=$CUDAHOME
export CPU_ENGINE=cpu
export CMDIO_PATH=/mnt/d/source/github/sim/mixlang/mlvm/src/cmdio
export LD_LIBRARY_PATH=/mnt/d/source/github/sim/mixlang/build/lib:$LD_LIBRARY_PATH

export CUDA_VERSION_NUMBER=${CUDA_VERSION}00

export PTX_SIM_USE_PTX_FILE=1.ptx
export PTX_SIM_KERNELFILE=_1.ptx
export PTX_SIM_MODE_FUNC=0
export PTX_SIM_DEBUG=3
export PYTORCH_BIN=
export CUOBJDUMP_SIM_FILE=jj
export PTX_JIT_PATH=
export LD_LIBRARY_PATH=$GEM5_ROOT/design/gpgpu/gpgpu-sim/libcuda:$LD_LIBRARY_PATH

# then update-alternative --config gcc/g++ to 4.7
# cd benchmarks/rodinia/vectorAdd or
# cd benchmarks/rodinia/backprop
# make gem5-fusion
