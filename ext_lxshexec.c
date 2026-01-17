/**
 * @file ext_lxshexec.c
 * @brief LX shell exec extension module.
 */
#include "lx_ext.h"
#include "lxsh_exec.h"
#include "value.h"
#include "array.h"
#include <stdlib.h>
#include <string.h>

static void array_clear(Array* a)
{
    if (!a) {
        return;
    }
    for (size_t i = 0; i < a->size; i++) {
        if (a->entries[i].key.type == KEY_STRING) {
            free(a->entries[i].key.s);
        }
        value_free(a->entries[i].value);
    }
    free(a->entries);
    a->entries = NULL;
    a->size = 0;
    a->capacity = 0;
}

static void array_push_line(Array* out, const char* line, size_t len, int stream_id)
{
    Value row = value_array();
    array_set(row.a, key_int(0), value_string_n(line, len));
    array_set(row.a, key_int(1), value_int(stream_id));
    lx_int_t idx = array_next_index(out);
    array_set(out, key_int(idx), row);
}

static Value n_exec(Env* env, int argc, Value* argv)
{
    (void)env;
    if (argc < 1) {
        return value_int(-1);
    }

    const LxShExecOps* ops = lxsh_get_exec_ops();
    if (!ops || !ops->exec) {
        return value_int(-1);
    }

    Value cv = value_to_string(argv[0]);
    const char* cmd = cv.s ? cv.s : "";

    Array* out = NULL;
    if (argc >= 2 && argv[1].type == VAL_ARRAY && argv[1].a) {
        out = argv[1].a;
        array_clear(out);
    }

    int status = 1;
    if (out && ops->exec_capture) {
        char* data = NULL;
        size_t len = 0;
        status = ops->exec_capture(cmd, &data, &len);
        if (data) {
            size_t start = 0;
            for (size_t i = 0; i <= len; ++i) {
                if (i == len || data[i] == '\n') {
                    size_t seg_len = i - start;
                    if (seg_len > 0 || i > start) {
                        array_push_line(out, data + start, seg_len, 1);
                    }
                    start = i + 1;
                }
            }
            free(data);
        }
    } else {
        status = ops->exec(cmd);
    }
    value_free(cv);
    return value_int(status);
}

static void lxshexec_module_init(Env* global)
{
    (void)global;
    lx_register_function("lxsh_exec", n_exec);
}

void register_lxshexec_module(void)
{
    lx_register_extension("lxshexec");
    lx_register_module(lxshexec_module_init);
}
