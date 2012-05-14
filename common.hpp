#ifndef __COMMON_GUARD
#define __COMMON_GUARD

#define unlikely(x) (x)
typedef unsigned long long word;
typedef __uint128_t dword;

#ifdef RELACY
#include "relacy/relacy_std.hpp"
// assert(x) is defined in relacy
// VAR_T is defined in relacy
// VAR is defined in relacy

#else

#include <cassert>
#include <atomic>
#define VAR_T(t) t
#define VAR(x) x

#endif

#endif

