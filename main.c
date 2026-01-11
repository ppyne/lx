/**
 * @file main.c
 * @brief Command-line entry point for running scripts.
 */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>

#include "lexer.h"
#include "parser.h"
#include "eval.h"
#include "env.h"
#include "natives.h"
#include "array.h"
#include "lx_ext.h"
#include "lx_error.h"
#include "lx_version.h"
#include "config.h"

#if LX_ENABLE_FS
void register_fs_module(void);
#endif
#if LX_ENABLE_JSON
void register_json_module(void);
#endif
#if LX_ENABLE_SERIALIZER
void register_serializer_module(void);
#endif
#if LX_ENABLE_HEX
void register_hex_module(void);
#endif
#if LX_ENABLE_BLAKE2B
void register_blake2b_module(void);
#endif
#if LX_ENABLE_TIME
void register_time_module(void);
#endif
#if LX_ENABLE_ENV
void register_env_module(void);
#endif
#if LX_ENABLE_UTF8
void register_utf8_module(void);
#endif
#if LX_ENABLE_SQLITE
void register_sqlite_module(void);
#endif

/* Stream reading utility. */
static char *read_stream(FILE *f) {
    size_t cap = 4096;
    size_t len = 0;
    char *buf = malloc(cap);
    if (!buf) return NULL;

    size_t n;
    while ((n = fread(buf + len, 1, cap - len, f)) > 0) {
        len += n;
        if (len == cap) {
            cap *= 2;
            char *tmp = realloc(buf, cap);
            if (!tmp) {
                free(buf);
                return NULL;
            }
            buf = tmp;
        }
    }

    buf[len] = '\0';
    return buf;
}

static void install_argv(Env *global, int argc, char **argv) {
    Value arr = value_array();
    for (int i = 1; i < argc; i++) {
        array_set(arr.a, key_int(i - 1), value_string(argv[i]));
    }
    env_set(global, "argc", value_int(argc > 0 ? argc - 1 : 0));
    env_set(global, "argv", arr);
}

static char *resolve_path(const char *path) {
    if (!path || !*path) return strdup("");
    char *real = realpath(path, NULL);
    if (real) return real;
    return strdup(path);
}

int main(int argc, char **argv) {

    char *source = NULL;
    char *filename = NULL;

    if (argc >= 2 && (!strcmp(argv[1], "-v") || !strcmp(argv[1], "--version"))) {
        printf("Lx %s\n", LX_VERSION_STRING);
        return 0;
    }

    if (!isatty(STDIN_FILENO)) {
        /* Read the script from stdin. */
        source = read_stream(stdin);
        if (!source) {
            fprintf(stderr, "error: cannot read stdin\n");
            return 1;
        }
        filename = strdup("<stdin>");
    } else {
        /* Read the script from a file. */
        if (argc < 2) {
            fprintf(stderr, "usage: %s script.lx [args]\n", argv[0]);
            return 1;
        }

        FILE *f = fopen(argv[1], "rb");
        if (!f) {
            fprintf(stderr, "error: cannot read file '%s'\n", argv[1]);
            return 1;
        }

        source = read_stream(f);
        fclose(f);

        if (!source) {
            fprintf(stderr, "error: cannot read file '%s'\n", argv[1]);
            return 1;
        }
        filename = resolve_path(argv[1]);
    }

    /* Initialize lexer and parser. */
    Parser parser;
    lexer_init(&parser.lexer, source, filename);
    parser.current.type = TOK_ERROR; /* force first advance */
    parser.previous.type = TOK_ERROR;
    lx_error_clear();

    /* Parse the program. */
    AstNode *program = parse_program(&parser);
    if (lx_has_error()) {
        lx_print_error(stderr);
        free(source);
        free(filename);
        return 1;
    }
    if (!program) {
        lx_print_error(stderr);
        free(source);
        free(filename);
        return 1;
    }

    /* Create the global environment. */
    Env *global = env_new(NULL);
    install_argv(global, argc, argv);

    /* Install the standard library. */
    install_stdlib();
#if LX_ENABLE_FS
    register_fs_module();
#endif
#if LX_ENABLE_JSON
    register_json_module();
#endif
#if LX_ENABLE_SERIALIZER
    register_serializer_module();
#endif
#if LX_ENABLE_HEX
    register_hex_module();
#endif
#if LX_ENABLE_BLAKE2B
    register_blake2b_module();
#endif
#if LX_ENABLE_TIME
    register_time_module();
#endif
#if LX_ENABLE_ENV
    register_env_module();
#endif
#if LX_ENABLE_UTF8
    register_utf8_module();
#endif
#if LX_ENABLE_SQLITE
    register_sqlite_module();
#endif
    /* Run extension initializers. */
    lx_init_modules(global);

    /* Execute. */
    EvalResult r = eval_program(program, global);
    if (lx_has_error()) {
        lx_print_error(stderr);
        value_free(r.value);
        env_free(global);
        free(source);
        free(filename);
        return 1;
    }

    /* Cleanup. */
    value_free(r.value);
    env_free(global);
    /* Free the AST if you add an ast_free() helper. */

    free(source);
    free(filename);
    return 0;
}
