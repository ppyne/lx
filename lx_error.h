/**
 * @file lx_error.h
 * @brief Global error reporting for parser and runtime.
 */
#ifndef LX_ERROR_H
#define LX_ERROR_H

#include <stdio.h>

/** Error codes for diagnostics. */
typedef enum {
    LX_ERR_NONE = 0,
    LX_ERR_PARSE = 1000,
    LX_ERR_RUNTIME = 2000,
    LX_ERR_DIV_ZERO = 2001,
    LX_ERR_MOD_ZERO = 2002,
    LX_ERR_UNDEFINED_FUNCTION = 2003,
    LX_ERR_INDEX_ASSIGN = 2004,
    LX_ERR_UNSET_TARGET = 2005,
    LX_ERR_BREAK_CONTINUE = 2006,
    LX_ERR_CYCLE = 2007,
    LX_ERR_INTERNAL = 9000
} LxErrorCode;

/** Error record. */
typedef struct {
    LxErrorCode code;
    int line;
    int col;
    char message[256];
} LxError;

/** Clear the current error state. */
void lx_error_clear(void);
/** @return Non-zero when an error is set. */
int lx_has_error(void);
/** @return Pointer to the current error record. */
const LxError *lx_get_error(void);
/** Set the current error with formatted message. */
void lx_set_error(LxErrorCode code, int line, int col, const char *fmt, ...);
/** Print the current error to @p out. */
void lx_print_error(FILE *out);

#endif
