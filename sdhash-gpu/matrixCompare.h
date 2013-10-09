#include <stdint.h>

#ifndef MATRIX_COMPARE
#define MATRIX_COMPARE
int matrixCompare(int block_size, int shortside, int long_A, int long_B, uint64_t *h_A, uint64_t *h_B, uint16_t *resptr, uint16_t *elem_A, uint16_t *elem_B,uint16_t *ham_A,uint16_t *ham_B);
int deviceSetup(int devID);
int deviceTeardown();

#define MAX_REF_SIZE 131072 // max bloom filter count
#define MIN_REF_SIZE 256 // gpu code behaves strangely w/r/t cutoffs if too small.

#endif