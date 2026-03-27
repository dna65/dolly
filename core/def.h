#pragma once

typedef unsigned char uchar_t;

#if defined(__GNUC__) || defined(__clang__)
#define __COLD__ __attribute__((cold))
#else
#define __COLD__
#endif
