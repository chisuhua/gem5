/*
 * Copyright (c) 2006 The Regents of The University of Michigan
 * All rights reserved.
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions are
 * met: redistributions of source code must retain the above copyright
 * notice, this list of conditions and the following disclaimer;
 * redistributions in binary form must reproduce the above copyright
 * notice, this list of conditions and the following disclaimer in the
 * documentation and/or other materials provided with the distribution;
 * neither the name of the copyright holders nor the names of its
 * contributors may be used to endorse or promote products derived from
 * this software without specific prior written permission.
 *
 * THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS
 * "AS IS" AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT
 * LIMITED TO, THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR
 * A PARTICULAR PURPOSE ARE DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT
 * OWNER OR CONTRIBUTORS BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL,
 * SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT
 * LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE,
 * DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY
 * THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT
 * (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE
 * OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
 *
 * Authors: Steve Reinhardt
 */

#include <stdio.h>

__global__ void
vectorCopy(const int *A, int *C, int numElements)
{
    int i = blockDim.x * blockIdx.x + threadIdx.x;

    if (i < numElements)
    {
        C[i] = A[i];
    }
}

int main(int argc, char* argv[])
{
    printf("Starting:!\n");
    int numElements = 32;
    cudaError_t err = cudaSuccess;
    size_t size = numElements * sizeof(float);

    int *h_A = (int *)malloc(size);
    int *h_A_from_d = (int *)malloc(size);
    int *h_C = (int *)malloc(size);

    // Verify that allocations succeeded
    if (h_A == NULL || h_A_from_d == NULL || h_C == NULL)
    {
        fprintf(stderr, "Failed to allocate host vectors!\n");
        exit(EXIT_FAILURE);
    }

    for (int i = 0; i < numElements; ++i)
    {
        h_A[i] = i;
        h_A_from_d[i] = 0;
        h_C[i] = 0;
    }


    int *d_A = NULL;
    err = cudaMalloc((void **)&d_A, size);
    if (err != cudaSuccess)
    {
        fprintf(stderr, "Failed to allocate device vector A (error code %s)!\n", cudaGetErrorString(err));
        exit(EXIT_FAILURE);
    }

    int *d_C = NULL;
    err = cudaMalloc((void **)&d_C, size);
    if (err != cudaSuccess)
    {
        fprintf(stderr, "Failed to allocate device vector A (error code %s)!\n", cudaGetErrorString(err));
        exit(EXIT_FAILURE);
    }

    printf("Allocate d_A %p, d_C %p!\n", d_A, d_C);

    err = cudaMemcpy(d_A, h_A, size, cudaMemcpyHostToDevice);
    if (err != cudaSuccess)
    {
        fprintf(stderr, "Failed to allocate device vector B (error code %s)!\n", cudaGetErrorString(err));
        exit(EXIT_FAILURE);
    }

#if 1
    err = cudaMemcpy(h_A_from_d, d_A, size, cudaMemcpyDeviceToHost);
    if (err != cudaSuccess)
    {
        fprintf(stderr, "Failed to allocate device vector B (error code %s)!\n", cudaGetErrorString(err));
        exit(EXIT_FAILURE);
    }

    bool compare_ret = true;
    for (int i = 0; i < numElements; ++i)
    {
        if (h_A[i] != h_A_from_d[i])
        {
            fprintf(stderr, "result compare failed on %d, expect %d, but get %d\n", i, h_A[i], h_A_from_d[i]);
            compare_ret = false;
        }
    }

    if (compare_ret != true) {
        printf("h_A copy compare failed!\n");
        exit(EXIT_FAILURE);
    } else {
        printf("h_A copy compare pass!\n");
    }
#endif
    // Launch the Vector Add CUDA Kernel
    int threadsPerBlock = 32;
    int blocksPerGrid =(numElements + threadsPerBlock - 1) / threadsPerBlock;
    printf("CUDA kernel launch with %d blocks of %d threads\n", blocksPerGrid, threadsPerBlock);
    vectorCopy<<<blocksPerGrid, threadsPerBlock>>>(d_A, d_C, numElements);
    err = cudaGetLastError();

    if (err != cudaSuccess)
    {
        fprintf(stderr, "Failed to launch vectorAdd kernel (error code %s)!\n", cudaGetErrorString(err));
        exit(EXIT_FAILURE);
    }

#if 1
    // Copy the device result vector in device memory to the host result vector
    // in host memory.
    printf("Copy output data from the CUDA device to the host memory\n");
    err = cudaMemcpy(h_C, d_C, size, cudaMemcpyDeviceToHost);
    if (err != cudaSuccess)
    {
        fprintf(stderr, "Failed to copy device vector C (error code %s)!\n", cudaGetErrorString(err));
        exit(EXIT_FAILURE);
    }


    bool fail = false;
    // Verify that the result vector is correct
    for (int i = 0; i < numElements; ++i)
    {
        printf("%d: A %d, C %d\n", i, h_A[i], h_C[i]);
        if (h_A[i] != h_C[i])
        {
            // fprintf(stderr, "Result verification failed at element %d!\n", i);
            fail = true;
            //exit(EXIT_FAILURE);
        }
    }

    if (fail) {
        fprintf(stderr, "Result verification failed at element\n");
    } else {
        printf("Test PASSED\n");
    }
#endif
    free(h_A);
    free(h_A_from_d);
    free(h_C);

    printf("Done\n");

    return 0;
}

