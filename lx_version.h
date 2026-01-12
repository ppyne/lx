/**
 * @file lx_version.h
 * @brief Lx version macros.
 */
#ifndef LX_VERSION_H
#define LX_VERSION_H

#define LX_VERSION_MAJOR 1
#define LX_VERSION_MINOR 2

#include "lx_version_build.h"

#ifndef LX_VERSION_BUILD
#define LX_VERSION_BUILD 0
#endif

#define LX_VERSION_STR_HELPER(x) #x
#define LX_VERSION_STR(x) LX_VERSION_STR_HELPER(x)
#define LX_VERSION_STRING LX_VERSION_STR(LX_VERSION_MAJOR) "." LX_VERSION_STR(LX_VERSION_MINOR) "." LX_VERSION_STR(LX_VERSION_BUILD)
#define LX_VERSION_NUM (LX_VERSION_MAJOR * 10000 + LX_VERSION_MINOR * 100 + LX_VERSION_BUILD)

#endif
