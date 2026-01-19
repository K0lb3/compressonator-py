/*
cmp_core requires these function to be defined,
so we have to provides stubs for archs that don't support them
(non x86)
*/
#include "core_simd.h"

float sse_bc1ComputeBestEndpoints(float *, float *, float *, float *, float *,
                                  int, int)
{
  throw "Not implemented";
}
float avx_bc1ComputeBestEndpoints(float *, float *, float *, float *, float *,
                                  int, int)
{
  throw "Not implemented";
}
float avx512_bc1ComputeBestEndpoints(float *, float *, float *, float *,
                                     float *, int, int)
{
  throw "Not implemented";
}

#ifdef _WIN32
#include <intrin.h>
#ifndef __cpuidex
extern "C"
{
  inline void __cpuidex(int CPUInfo[4], int function_id, int subfunction_id)
  {
    (void)function_id;
    (void)subfunction_id;
    CPUInfo[0] = CPUInfo[1] = CPUInfo[2] = CPUInfo[3] = 0;
  }
}
#endif