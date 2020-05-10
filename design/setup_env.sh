DIR="$( cd "$( dirname "${BASH_SOURCE[0]}" )" && cd .. && pwd )"
# is is used to build benchmark
# 
export GEM5_ROOT=$DIR
export CUDAHOME=/usr/local/cuda/
#export CUDAHOME=$DIR/cuda3.2
export PATH="$CUDAHOME/bin:$PATH"
export CUDA_INSTALL_PATH=$CUDAHOME


# then update-alternative --config gcc/g++ to 4.7
# cd benchmarks/rodinia/vectorAdd or
# cd benchmarks/rodinia/backprop
# make gem5-fusion
