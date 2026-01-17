/**
 * @file ext_lxshcli.c
 * @brief LX shell input helpers.
 */
#include "lx_ext.h"
#include "lxsh_runtime.h"
#include "lx_error.h"
#include "value.h"
#include <stdlib.h>

int lxsh_cli_read_key(int* out_code);
char* lxsh_cli_read_line(void);
void lxsh_cli_prompt(const char* prompt);

static void print_prompt(Value prompt)
{
    Value pv = value_to_string(prompt);
    lxsh_cli_prompt(pv.s ? pv.s : "");
    value_free(pv);
}

static Value n_lxsh_read_line(Env* env, int argc, Value* argv)
{
    (void)env;
    if (argc > 1) {
        return value_undefined();
    }
    if (argc == 1) {
        print_prompt(argv[0]);
    }

    char* line = lxsh_cli_read_line();
    if (!line) {
        if (lxsh_cancel_requested()) {
            lx_set_error(LX_ERR_RUNTIME, 0, 0, "interrupted");
        }
        return value_undefined();
    }
    Value out = value_string(line);
    free(line);
    return out;
}

static Value n_lxsh_read_key(Env* env, int argc, Value* argv)
{
    (void)env;
    if (argc > 1) {
        return value_undefined();
    }
    if (argc == 1) {
        print_prompt(argv[0]);
    }

    int code = 0;
    if (!lxsh_cli_read_key(&code)) {
        if (lxsh_cancel_requested()) {
            lx_set_error(LX_ERR_RUNTIME, 0, 0, "interrupted");
        }
        return value_undefined();
    }
    return value_int(code);
}

static void lxshcli_module_init(Env* global)
{
    (void)global;
    lx_register_function("lxsh_read_line", n_lxsh_read_line);
    lx_register_function("lxsh_read_key", n_lxsh_read_key);
}

void register_lxshcli_module(void)
{
    lx_register_extension("lxshcli");
    lx_register_module(lxshcli_module_init);
}
