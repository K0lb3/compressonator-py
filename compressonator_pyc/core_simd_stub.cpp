/*
cmp_core requires these function to be defined,
so we have to provides stubs for archs that don't support them
(non x86)
*/
#include "core_simd.h"

float sse_bc1ComputeBestEndpoints(float *, float *, float *, float *, float *,
                                  int, int) {
  throw "Not implemented";
}
float avx_bc1ComputeBestEndpoints(float *, float *, float *, float *, float *,
                                  int, int) {
  throw "Not implemented";
}
float avx512_bc1ComputeBestEndpoints(float *, float *, float *, float *,
                                     float *, int, int) {
  throw "Not implemented";
}
