#pragma once
#include <cstddef>
#include <cstdlib>

// Force NULL to be 0 for boolean comparisons in legacy source files
#pragma push_macro("NULL")
#undef NULL
#define NULL 0
