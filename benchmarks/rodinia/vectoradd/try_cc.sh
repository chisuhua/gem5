#nvcc -DOUTPUT -I/mnt/d/source/github/sim/gem5-gpu/gem5_benchmark/cuda3.2/include  -O3 -c -arch sm_20 --keep --compiler-options -fno-strict-aliasing \
#	-DGEM5_FUSION -I. -I/mnt/d/source/github/sim/gem5-gpu/gem5_benchmark/cuda3.2/include/ -I/common/inc/ \
#	 -I../../../gem5/util/m5 -I../../libcuda -L/lib -lcutil -DUNIX vectorAdd.cu -o vectorAdd


#clang++-9 -DOUTPUT -I/mnt/d/source/github/sim/gem5-gpu/gem5_benchmark/cuda3.2/include  -O3 -c -fno-strict-aliasing \
#	--cuda-gpu-arch=sm_30 --cuda-path=/usr/local/cuda-9.2 -L/usr/local/cuda-9.2/lib64 \
#	-DGEM5_FUSION -I. -I/mnt/d/source/github/sim/gem5-gpu/gem5_benchmark/cuda3.2/include/ -I/common/inc/ \
#	 -I../../../gem5/util/m5 -I../../libcuda -L/lib -DUNIX vectorAdd.cu -o vectorAdd

clang++-9 simple.cu --cuda-gpu-arch=sm_30 --cuda-path=/usr/local/cuda-9.2 -L/usr/local/cuda-9.2/lib64 -lcudart -pthread -o simple

#clang++-9 -DOUTPUT -O3 -c -fno-strict-aliasing \
#	--cuda-gpu-arch=sm_30 --cuda-path=/usr/local/cuda-9.2 -L/usr/local/cuda-9.2/lib64 \
#	-DGEM5_FUSION -I. -I/common/inc/ \
#	 -I../../../gem5/util/m5 -I../../libcuda -L/lib -DUNIX vectorAdd.cu -o vectorAdd
