/* 
 * File:   test.cu
 */

#include <test.h>

__device__ void hello()
{
    int a = 1 + 10;
    return;
}

__global__ 
void hellok() 
{
    hello();
}

__host__
void testcuda()
{
    hellok<<<10,10>>>();
}

/*
 * 
 */