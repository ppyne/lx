#ifndef LXSH_FS_H
#define LXSH_FS_H

#include <stddef.h>

typedef struct {
    int (*read_file)(const char *path, char **out_data, size_t *out_len,
                     char **out_resolved);
    int (*write_file)(const char *path, const unsigned char *data, size_t len);
    int (*file_exists)(const char *path);
    int (*file_size)(const char *path, size_t *out_size);
    int (*is_dir)(const char *path);
    int (*is_file)(const char *path);
    int (*mkdir)(const char *path);
    int (*rmdir)(const char *path);
    int (*unlink)(const char *path);
    int (*copy)(const char *src, const char *dst);
    int (*rename)(const char *src, const char *dst);
    int (*pwd)(char *out, size_t out_sz);
    int (*list_dir)(const char *path, char ***out_names, size_t *out_count);
    const char *(*temp_dir)(void);
    int (*tempnam)(const char *prefix, char *out, size_t out_sz);
} LxShFsOps;

void lxsh_set_fs_ops(const LxShFsOps *ops);
const LxShFsOps *lxsh_get_fs_ops(void);

#endif
