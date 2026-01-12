/**
 * @file lx_int.h
 * @brief Integer width configuration for Lx.
 */
#ifndef LX_INT_H
#define LX_INT_H

#include "config.h"
#include <limits.h>

#if !defined(LX_INT_BITS)
#define LX_INT_BITS 64
#endif

#if LX_INT_BITS == 64
typedef long long lx_int_t;
typedef unsigned long long lx_uint_t;
#define LX_INT_MAX LLONG_MAX
#define LX_INT_MIN LLONG_MIN
#define LX_INT_FMT "lld"
#define LX_UINT_FMT "llu"
#else
typedef int lx_int_t;
typedef unsigned int lx_uint_t;
#define LX_INT_MAX INT_MAX
#define LX_INT_MIN INT_MIN
#define LX_INT_FMT "d"
#define LX_UINT_FMT "u"
#endif

#define LX_INT_SIZE ((int)sizeof(lx_int_t))

#endif
