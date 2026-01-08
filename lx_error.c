/**
 * @file lx_error.c
 * @brief Global error reporting for parser and runtime.
 */
#include "lx_error.h"
#include <stdarg.h>
#include <string.h>

static LxError g_error = { LX_ERR_NONE, 0, 0, {0} };

void lx_error_clear(void) {
    g_error.code = LX_ERR_NONE;
    g_error.line = 0;
    g_error.col = 0;
    g_error.message[0] = '\0';
}

int lx_has_error(void) {
    return g_error.code != LX_ERR_NONE;
}

const LxError *lx_get_error(void) {
    return &g_error;
}

void lx_set_error(LxErrorCode code, int line, int col, const char *fmt, ...) {
    if (code == LX_ERR_NONE) return;
    g_error.code = code;
    g_error.line = line;
    g_error.col = col;

    va_list ap;
    va_start(ap, fmt);
    vsnprintf(g_error.message, sizeof(g_error.message), fmt, ap);
    va_end(ap);
}

void lx_print_error(FILE *out) {
    if (!lx_has_error()) return;
    if (g_error.line > 0) {
        fprintf(out, "error %d line %d:%d: %s\n",
                g_error.code, g_error.line, g_error.col, g_error.message);
    } else {
        fprintf(out, "error %d: %s\n", g_error.code, g_error.message);
    }
}
