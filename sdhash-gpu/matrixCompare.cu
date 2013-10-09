// matrix bloom filter comparison
// author: candice quates
//
////////////////////////////////////////////////////////////////////////////
// Portions of this program were derived from cuda toolkit samples,
// specifically the shared-memory version of matrix multiplication,
// and the repeated range sample code.
//
// Those parts are subject to the nvidia EULA as below.
// 


///////////////////////////////////////////////////////////////////////////
// Copyright 1993-2012 NVIDIA Corporation.  All rights reserved.
//
// Please refer to the NVIDIA end user license agreement (EULA) associated
// with this source code for terms and conditions that govern your use of
// this software. Any use, reproduction, disclosure, or distribution of
// this software and related documentation outside the terms of the EULA
// is strictly prohibited.
///


// includes, system
#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <math.h>
#include <stdint.h>

// includes CUDA
#include <cuda_runtime.h>

#define MIN_FULLNESS 16

// includes, project
//#include <helper_cuda.h>
//#include <helper_functions.h> // helper functions for SDK examples

// thrust for reductions -- smaller reductions are actually okay.
#include <cuda.h>

#include "matrixCompare.h"
#include <sdbf_class.h>
#include <bloom_filter.h>

#include <sdbf_set.h>
/**
 * Comparison (CUDA Kernel) on the device: C = popcll( A & B )
 * wA is A's width and wB is B's width
 */
template <int BLOCK_SIZE> __global__ void
CompKernelShared(uint16_t *C, uint64_t *A, uint64_t *B, int wA, int wB)
{
    // Block index
    int bx = blockIdx.x;
    int by = blockIdx.y;

    // Thread index
    int tx = threadIdx.x;
    int ty = threadIdx.y;

    // Index of the first sub-matrix of A processed by the block
    int aBegin = wA * BLOCK_SIZE * by;

    // Index of the last sub-matrix of A processed by the block
    int aEnd   = aBegin + wA - 1;

    // Step size used to iterate through the sub-matrices of A
    int aStep  = BLOCK_SIZE;

    // Index of the first sub-matrix of B processed by the block
    int bBegin = BLOCK_SIZE * bx;

    // Step size used to iterate through the sub-matrices of B
    int bStep  = BLOCK_SIZE * wB;

    // Csub is used to store the element of the block sub-matrix
    // that is computed by the thread
    uint64_t Csub = 0;

    // Loop over all the sub-matrices of A and B
    // required to compute the block sub-matrix
    for (int a = aBegin, b = bBegin;
         a <= aEnd;
         a += aStep, b += bStep)
    {

        // Declaration of the shared memory array As used to
        // store the sub-matrix of A
        __shared__ uint64_t As[BLOCK_SIZE][BLOCK_SIZE];

        // Declaration of the shared memory array Bs used to
        // store the sub-matrix of B
        __shared__ uint64_t Bs[BLOCK_SIZE][BLOCK_SIZE];

        // Load the matrices from device memory
        // to shared memory; each thread loads
        // one element of each matrix
        As[ty][tx] = A[a + wA * ty + tx];
        Bs[ty][tx] = B[b + wB * ty + tx];

        // Synchronize to make sure the matrices are loaded
        __syncthreads();

        // Compare the two matrices together;
        // each thread computes one element
        // of the block sub-matrix
#pragma unroll

        for (int k = 0; k < BLOCK_SIZE; ++k)
        {
			// checking for 0 less efficient than computation. less divergence.
			Csub += __popcll(As[ty][k] & Bs[k][tx]);
        }

        // Synchronize to make sure that the preceding
        // computation is done before loading two new
        // sub-matrices of A and B in the next iteration
        __syncthreads();
    }

    // Write the block sub-matrix to device memory;
    // each thread writes one element
    int c = wB * BLOCK_SIZE * by + BLOCK_SIZE * bx;
    C[c + wB * ty + tx] = Csub;
}

// kernel for minimum estimate cache
__global__ void EstCacheKernel(uint16_t *R) {
	double m = 2048;
	double k = 5;
	double exp = 1-1.0/2048;
	int x = blockIdx.x * blockDim.x + threadIdx.x;
	int y = blockIdx.y * blockDim.y + threadIdx.y;
	// bloom filter minimum estimate calculation for all element counts up to 256
	R[x*256+y]=(uint16_t)llrintf(m*(1- powf(exp,k*x) - powf(exp,k*y)+ powf(exp,k*(x+y))) );
	
}

// compute and apply cutoff based upon hamming weight and fullness of filters
// 1 and 2 apply to A and B-matrixes respectively.
__global__ void CutoffKernel(uint16_t *R, uint16_t *cache, uint16_t *s1, uint16_t *s2, uint16_t *ham1, uint16_t *ham2, int longsideB) {
	int row = blockIdx.y * blockDim.y + threadIdx.y;
    int col = blockIdx.x * blockDim.x + threadIdx.x;
	// if either filter does not have enough elements
	// throw out the result
	int min_est=0;
	int s1row=s1[row];
	int s2col=s2[col];
	if ((s1row < MIN_FULLNESS ) || (s2col < MIN_FULLNESS)) {
		R[row * longsideB + col]=0;
	} else {
		if (s1row==160 && s2col==160) // cache hit avoidance; most common case
			min_est=214;
		else 
			min_est=cache[s1row*256+s2col];	// expensive
		int max_est = (ham1[row] < ham2[col]) ? ham1[row]: ham2[col];
		float cut_off=(0.3*(float)(max_est-min_est)+(float)min_est);
		R[row * longsideB + col] = (R[row * longsideB + col] > cut_off)? (uint16_t)llrintf(100*(R[row * longsideB + col]-cut_off)/(max_est-cut_off)) : 0 ;
	}
}

// reduction kernel -- basic but still faster than reduce-by-key
// strided accesses etc probably would help.
__global__ void
ReduceKernel(uint16_t *A, uint16_t *C, int wA)
{
    // Each thread computes one element of C
	// from each row of A
    uint16_t Cmax = 0;
    int row = blockIdx.x * blockDim.x + threadIdx.x;
	for (int e = 0; e < wA; ++e) {
		if (A[row * wA +e] > Cmax)
			Cmax = A[row * wA + e];
	}
    C[row] = Cmax;
}

void constantInitC(uint64_t *data, int size, uint64_t val)
{
    for (int i = 0; i < size; ++i)
        data[i] = val;
}

void constantInitCs(uint16_t *data, int size, uint16_t val)
{
    for (int i = 0; i < size; ++i)
        data[i] = val;
}
int nextpower2(int32_t bf_count) {
	int32_t v = bf_count;
	v--;
	v |= v >> 1;
	v |= v >> 2;
	v |= v >> 4;
	v |= v >> 8;
	v++;
	if (v < 32 ) v = 32;
	return v;
}

// wrapper bit for matrixCompare to compare sdbf sets
// 
int sdbfsetCompare(sdbf_set *refset, sdbf_set *target, bool quiet, int confidence) {    
    
	int shortside=32;

	// allocate these based on target sizing
	int longsidea=32768;
	int longsideb=32;

	int block_size=32;
	longsidea = nextpower2(refset->bf_vector->size());
	//longsidea *=4;
	if (!quiet)
	   std::cout <<"side A " << longsidea << std::endl;

    // Allocate host memory for matrices A and B
    unsigned int size_A = shortside *longsidea;
    unsigned int mem_size_A = sizeof(uint64_t) * size_A;
    uint64_t *h_A = (uint64_t *)malloc(mem_size_A);

	// Initialize host memory - ie make some data
    constantInitC(h_A, size_A, 0);
	uint8_t *h_A8;
	h_A8=(uint8_t*)h_A;
	uint16_t *ham_A = (uint16_t*)malloc(longsidea*sizeof(uint16_t));
    constantInitCs(ham_A, longsidea, 0);
	uint16_t *elem_A = (uint16_t*)malloc(longsidea*sizeof(uint16_t));
    constantInitCs(elem_A, longsidea, 0);
    // load bloom filters into A 
	for (int i=0; i < refset->bf_vector->size(); i++) {
		for (int j=0; j < 256; j++) {
			h_A8[256*i+j]=refset->bf_vector->at(i)->bf[j];
		}
		ham_A[i]=refset->bf_vector->at(i)->hamming;
		elem_A[i]=(uint16_t)refset->bf_vector->at(i)->elem_count();
	}
	


	unsigned int mem_size_R = longsidea * sizeof(uint16_t);
	uint16_t *results = (uint16_t *) malloc(mem_size_R);
	// B we will load repeatedly with one sdbf at a time
	// from target
    // allocate device memory for A parts
	// and transfer it up to the device.
	uint64_t *d_A, *d_B;
    cudaError_t error;
    error = cudaMalloc((void **) &d_A, mem_size_A);
    if (error != cudaSuccess)    {
        printf("cudaMalloc d_A returned error code %d, line(%d)\n", error, __LINE__);
        exit(EXIT_FAILURE);
    }
    error = cudaMemcpy(d_A, h_A, mem_size_A, cudaMemcpyHostToDevice);
    if (error != cudaSuccess)    {
        printf("cudaMemcpy (d_A,h_A) returned error code %d, line(%d)\n", error, __LINE__);
        exit(EXIT_FAILURE);
    }
	int mem_size_Act = longsidea*sizeof(uint16_t);
	uint16_t *ham_Ad, *elem_Ad;
	error = cudaMalloc((void **) &elem_Ad, mem_size_Act);
	if (error != cudaSuccess)    {
		printf("cudaMalloc d_B returned error code %d, line(%d)\n", error, __LINE__);
		exit(EXIT_FAILURE);
	}  	
	error = cudaMalloc((void **) &ham_Ad, mem_size_Act);
	if (error != cudaSuccess)    {
		printf("cudaMalloc d_B returned error code %d, line(%d)\n", error, __LINE__);
		exit(EXIT_FAILURE);
	}  	
	error = cudaMemcpy(ham_Ad, ham_A, mem_size_Act, cudaMemcpyHostToDevice);
	if (error != cudaSuccess)    {
		printf("cudaMemcpy (d_B,h_B) returned error code %d, line(%d)\n", error, __LINE__);
		exit(EXIT_FAILURE);
	}
	error = cudaMemcpy(elem_Ad, elem_A, mem_size_Act, cudaMemcpyHostToDevice);
	if (error != cudaSuccess)    {
		printf("cudaMemcpy (d_B,h_B) returned error code %d, line(%d)\n", error, __LINE__);
		exit(EXIT_FAILURE);
	}
	// for each sdbf in set2, load into B and compare it with A.
	for (int m=0; m < target->size(); m++) {
		longsideb=1024; // fix this
		longsideb = nextpower2(target->at(m)->filter_count());
		if (!quiet)
			std::cout <<"side B " << longsideb << std::endl;
		unsigned int size_B = shortside *longsideb;
		unsigned int mem_size_B = sizeof(uint64_t) * size_B;
		uint64_t *h_B = (uint64_t *)malloc(mem_size_B);
		constantInitC(h_B, size_B, 0);
		uint8_t *h_B8;
		h_B8=(uint8_t*)h_B;    
		
		int mem_size_Bct =sizeof(uint16_t)*longsideb;
		
		uint16_t *ham_B = (uint16_t*)malloc(mem_size_Bct);
		constantInitCs(ham_B, longsideb, 0);
		uint16_t *elem_B = (uint16_t*)malloc(mem_size_Bct);
		constantInitCs(elem_B, longsideb, 0);
		uint8_t *tmpbuf;
		int max_elem=0;
	    for (int j=0; j < target->at(m)->filter_count() ; j++) {
			ham_B[j]=target->at(m)->hamming[j];
			elem_B[j]=sdbf::get_elem_count(target->at(m),j);
			if (elem_B[j] > max_elem) max_elem=elem_B[j];			                  
            for (int i=0; i< 256; i+=8) {
                for (int k=0;k<8;k++)
					h_B8[i*longsideb+k+j*8]=target->at(m)->buffer[i+k+j*256];
            }
		}
		// if the whole block has too few elements, skip it.
		// for filtering out empty parts of drives
		if (max_elem < MIN_FULLNESS) {
			free(h_B);
			free(ham_B);
			free(elem_B);
			continue;
		}

		uint16_t *ham_Bd, *elem_Bd;
		error = cudaMalloc((void **) &elem_Bd, mem_size_Bct);
		if (error != cudaSuccess)    {
			printf("cudaMalloc d_B returned error code %d, line(%d)\n", error, __LINE__);
			exit(EXIT_FAILURE);
		}  	
		error = cudaMalloc((void **) &ham_Bd, mem_size_Bct);
		if (error != cudaSuccess)    {
			printf("cudaMalloc d_B returned error code %d, line(%d)\n", error, __LINE__);
			exit(EXIT_FAILURE);
		}  	
		error = cudaMemcpy(ham_Bd, ham_B, mem_size_Bct, cudaMemcpyHostToDevice);
		if (error != cudaSuccess)    {
			printf("cudaMemcpy (d_B,h_B) returned error code %d, line(%d)\n", error, __LINE__);
			exit(EXIT_FAILURE);
		}
		error = cudaMemcpy(elem_Bd, elem_B, mem_size_Bct, cudaMemcpyHostToDevice);
		if (error != cudaSuccess)    {
			printf("cudaMemcpy (d_B,h_B) returned error code %d, line(%d)\n", error, __LINE__);
			exit(EXIT_FAILURE);
		}

		error = cudaMalloc((void **) &d_B, mem_size_B);
		if (error != cudaSuccess)    {
			printf("cudaMalloc d_B returned error code %d, line(%d)\n", error, __LINE__);
			exit(EXIT_FAILURE);
		}  	
		error = cudaMemcpy(d_B, h_B, mem_size_B, cudaMemcpyHostToDevice);
		if (error != cudaSuccess)    {
			printf("cudaMemcpy (d_B,h_B) returned error code %d, line(%d)\n", error, __LINE__);
			exit(EXIT_FAILURE);
		}
		int matrix_result;
		matrix_result = matrixCompare(block_size, shortside, longsidea,longsideb, d_A, d_B, results, elem_Ad, elem_Bd, ham_Ad, ham_Bd);
		int result_count=0;
		int score=0;
		int filter_count=0;
		if (!quiet)
		    std::cout << target->at(m)->name()  << std::endl;
		int setsize =refset->bf_vector->size();
		for (int i=0; i < setsize; i++) {
			if (results[i]>0) { 
			    result_count++;
			    score+=results[i];
			}
			if (elem_A[i]>=MIN_FULLNESS)
				filter_count++;
			if (i+1 == setsize) {
				if (result_count!=0  && (score/filter_count >= confidence)) {
					cout << target->at(m)->name()  << "|"<< refset->bf_vector->at(i)->name() << "|" << score/filter_count<< endl;
					result_count=0;
					score=0;
				}
			} else if (refset->bf_vector->at(i)->bloom_id() != refset->bf_vector->at(i+1)->bloom_id()) {
				if (result_count!=0 && (score/filter_count >= confidence)) {	
					cout << target->at(m)->name()  << "|"<< refset->bf_vector->at(i)->name() << "|" << score/filter_count<< endl;		
				}
				filter_count=0;
				result_count=0;
				score=0;
			}
		}	

		free(h_B);
		free(ham_B);
		free(elem_B);
		cudaFree(d_B);
		cudaFree(ham_Bd);
		cudaFree(elem_Bd);
	}
	cudaFree(d_A);
	free(ham_A);
	free(elem_A);
	free(h_A);
	cudaFree(ham_Ad);
	cudaFree(elem_Ad);
	free(results);
	return 0;
}

// Compares two matrices in device memory.
// host required to pre-allocate result memory in *resptr
int matrixCompare(int block_size, int shortside, int long_A, int long_B, uint64_t *d_A, uint64_t *d_B, uint16_t *resptr, 
	uint16_t *elem_A, uint16_t *elem_B,uint16_t *ham_A,uint16_t *ham_B)
{
	
    dim3 dimsA(shortside,long_A, 1);
    dim3 dimsB(long_B, shortside, 1);
    // size calcs
    unsigned int size_A = dimsA.x * dimsA.y;
    unsigned int mem_size_A = sizeof(uint64_t) * size_A;
    unsigned int size_B = dimsB.x * dimsB.y;
    unsigned int mem_size_B = sizeof(uint64_t) * size_B;

    // Size  matrix C 
    dim3 dimsC(dimsB.x, dimsA.y, 1);
    unsigned int size_C = dimsC.x * dimsC.y;
    unsigned int mem_size_C = dimsC.x * dimsC.y * sizeof(uint16_t);
	
    unsigned int mem_size_R = dimsA.y * sizeof(uint16_t);
    //uint16_t *h_C = (uint16_t *) malloc(mem_size_C);
	
    // Declare device memory
    uint16_t *d_C;
	
    // allocate device memory for computation matrix
    cudaError_t error;
    error = cudaMalloc((void **) &d_C, mem_size_C);
    if (error != cudaSuccess)    {
        printf("cudaMalloc d_C returned error code %d, line(%d)\n", error, __LINE__);
        exit(EXIT_FAILURE);
    }

	uint16_t *d_cache;
	error = cudaMalloc((void **) &d_cache, 256*256*(sizeof(uint16_t)));
	if (error != cudaSuccess)    {
		printf("cudaMalloc est_cache returned error code %d, line(%d)\n", error, __LINE__);
		exit(EXIT_FAILURE);
	}

	// compute cutoff cache values
	dim3 threadscache(16,16);
	dim3 gridcache(256/threadscache.x,256/threadscache.y);
	EstCacheKernel<<<gridcache,threadscache>>>(d_cache);   
    // Setup thread block and grid
    dim3 threads(block_size, block_size);
    dim3 grid(dimsB.x / threads.x, dimsA.y / threads.y);
    // Compare Kernel 
    CompKernelShared<32><<< grid, threads>>>(d_C, d_A, d_B, dimsA.x, dimsB.x);
	// cutoff kernel
	CutoffKernel<<<grid,threads>>>(d_C, d_cache, elem_A, elem_B, ham_A, ham_B, dimsC.x);

	// allocate result space
	uint16_t *d_R;
	error = cudaMalloc((void **) &d_R, mem_size_R);
    if (error != cudaSuccess)    {
        printf("cudaMalloc d_C returned error code %d, line(%d)\n", error, __LINE__);
        exit(EXIT_FAILURE);
    }
	//reduction kernel -- runs on 1-dimension not in a grid shape like the others
	//ReduceKernel<<<dimsC.y/1024,1024>>>(d_C, d_R, dimsC.x);
	ReduceKernel<<<dimsC.y/16,16>>>(d_C, d_R, dimsC.x);
    error = cudaMemcpy(resptr, d_R, mem_size_R, cudaMemcpyDeviceToHost);
    if (error != cudaSuccess)    {
        printf("cudaMemcpy (h_R,d_R) returned error code %d, line(%d)\n", error, __LINE__);
        exit(EXIT_FAILURE);
    }    	

	//for (int i=0; i < dimsC.y; i++) {
	//	cout << resptr[i] << " " ;
	//}	
    
    cudaFree(d_C);
	cudaFree(d_R);
	cudaFree(d_cache);
    return EXIT_SUCCESS;
    
}

int deviceSetup(int devID) {
    cudaSetDevice(devID);
    cudaError_t error;
    cudaDeviceProp deviceProp;
    error = cudaGetDevice(&devID);

    if (error != cudaSuccess)    {
        fprintf(stderr,"cudaGetDevice returned error code %d, line(%d)\n", error, __LINE__);
    }

    error = cudaGetDeviceProperties(&deviceProp, devID);
    if (deviceProp.computeMode == cudaComputeModeProhibited)    {
        fprintf(stderr, "Error: device is running in <Compute Mode Prohibited>, no threads can use ::cudaSetDevice().\n");
        return(1);
    }

    if (error != cudaSuccess)    {
        fprintf(stderr,"cudaGetDeviceProperties returned error code %d, line(%d)\n", error, __LINE__);
    }
    else    {
        fprintf(stderr,"GPU Device %d: \"%s\" with compute capability %d.%d\n\n", devID, deviceProp.name, deviceProp.major, deviceProp.minor);
    }
	return 0;
}

int deviceTeardown() {
	cudaDeviceReset(); // at end?
	return 0;
}
