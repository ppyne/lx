#include "lxsh_fs.h"

static const LxShFsOps *g_ops = NULL;

void lxsh_set_fs_ops(const LxShFsOps *ops) {
    g_ops = ops;
}

const LxShFsOps *lxsh_get_fs_ops(void) {
    return g_ops;
}
