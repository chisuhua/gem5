DIR="$( cd "$( dirname "${BASH_SOURCE[0]}" )" && cd .. && pwd )"
# is is used to build benchmark
#
if [ x"$PPFLOW_ROOT" == "x" ];
then
	PPFLOW_ROOT=$DIR/../../mixlang
fi

export GEM5_ROOT=$DIR
export CUDAHOME=/usr/local/cuda/
#export CUDAHOME=$DIR/cuda3.2
export PATH="$CUDAHOME/bin:$PATH"
export CUDA_INSTALL_PATH=$CUDAHOME
export CPU_ENGINE=cpu
export CMDIO_PATH=/mnt/d/source/github/sim/mixlang/mlvm/src/cmdio
export LD_LIBRARY_PATH=/mnt/d/source/github/sim/mixlang/build/lib:$LD_LIBRARY_PATH


# then update-alternative --config gcc/g++ to 4.7
# cd benchmarks/rodinia/vectorAdd or
# cd benchmarks/rodinia/backprop
# make gem5-fusion
