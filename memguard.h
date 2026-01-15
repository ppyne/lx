/**
 * @file memguard.h
 * @brief Runtime memory guard helpers.
 */
#ifndef MEMGUARD_H
#define MEMGUARD_H

#include <stddef.h>

#ifdef __cplusplus
extern "C" {
#endif

size_t lx_platform_free_heap(void);
void lx_set_mem_reserve(size_t bytes);
size_t lx_get_mem_reserve(void);
int lx_memguard_check(size_t want_bytes);

#ifdef __cplusplus
}
#endif

#endif
