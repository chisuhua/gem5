# vector.c is just quick verify by gcc compile

#nvcc -DOUTPUT -I/mnt/d/source/github/sim/gem5-gpu/gem5_benchmark/cuda3.2/include  -O3 -c -arch sm_20 --keep --compiler-options -fno-strict-aliasing \
#	-DGEM5_FUSION -I. -I/mnt/d/source/github/sim/gem5-gpu/gem5_benchmark/cuda3.2/include/ -I/common/inc/ \
#	 -I../../../gem5/util/m5 -I../../libcuda -L/lib -lcutil -DUNIX vectorAdd.cu -o vectorAdd

g++ -DOUTPUT -I/mnt/d/source/github/sim/gem5-gpu/gem5_benchmark/cuda3.2/include -g -c -O1 \
	-DGEM5_FUSION -I. -I/common/inc/ \
	-I../../libcuda -DUNIX vectorAdd.c -o vectorAdd.o


g++ -DOUTPUT -I. -O3 -DGEM5_FUSION  vectorAdd.o -L../../libcuda -lcuda -L/lib -lz  -static -static-libgcc -o vectorAdd -L/usr/lib64 -lm5op_x86 -lm5op_x86 -lm -lc

#python ../../common/sizeHack.py -f vectorAdd.cu.cpp -t sm_20
#('the place is', 652695)
#change will be:
#or__));
#static void __sti____cudaRegisterAll_17_vectorAdd_cpp1_ii_main(void){__cudaFatCubinHandle = __cudaRegisterFatBinary2((void*)(&__fatDeviceText), sizeof(__deviceText_$compute_20$)); atexit(__cudaUnregisterBinaryUtil);}
# 1 "vectorAdd.cudafe1.stub.c" 2

#g++-4.7 -DOUTPUT -I. -O3 -I/mnt/d/source/github/sim/gem5-gpu/gem5_benchmark/cuda3.2/include -g -c vectorAdd.cu.cpp -o vectorAdd.cu_o
#g++-4.7 -DOUTPUT -I. -O3 -DGEM5_FUSION vectorAdd.cu_o -L../../libcuda -lcuda \
#	-L/lib \
#	-lz -static -static-libgcc -o gem5_fusion_vectorAdd -L/usr/lib64 -lm5op_x86 -lm -lc  
