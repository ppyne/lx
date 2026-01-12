/**
 * @file ext_serializer.h
 * @brief Serializer helpers for C integration.
 */
#ifndef EXT_SERIALIZER_H
#define EXT_SERIALIZER_H

#include "value.h"

int lx_serialize(Value v, char **out, size_t *len);
Value lx_unserialize_string(const char *s, int *ok);

#endif
