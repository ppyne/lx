#pragma once

#include <stddef.h>

#ifdef __cplusplus
extern "C" {
#endif

typedef struct {
    int (*exec)(const char* command);
    int (*exec_capture)(const char* command, char** out_data, size_t* out_len);
} LxShExecOps;

void lxsh_set_exec_ops(const LxShExecOps* ops);
const LxShExecOps* lxsh_get_exec_ops(void);

#ifdef __cplusplus
}
#endif
