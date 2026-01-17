#include "lxsh_exec.h"
#include <stddef.h>

static const LxShExecOps* g_ops = NULL;

void lxsh_set_exec_ops(const LxShExecOps* ops)
{
    g_ops = ops;
}

const LxShExecOps* lxsh_get_exec_ops(void)
{
    return g_ops;
}
