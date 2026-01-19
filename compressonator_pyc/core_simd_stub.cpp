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

#ifdef IMPL__cpuidex
void __cpuidex(int cpuInfo[4],
               int function_id,
               int subfunction_id)
{
  (void)function_id;
  (void)subfunction_id;
  cpuInfo[0] = 0;
  cpuInfo[1] = 0;
  cpuInfo[2] = 0;
  cpuInfo[3] = 0;
}
#undef IMPL__cpuidex
#endif
