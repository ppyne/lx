/**
 * @file main.c
 * @brief Command-line entry point for running scripts.
 */

#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>

#include "lexer.h"
#include "parser.h"
#include "eval.h"
#include "env.h"
#include "natives.h"
#include "lx_ext.h"
#include "lx_error.h"

void register_fs_module(void);
void register_json_module(void);
void register_serializer_module(void);
void register_hex_module(void);
void register_blake2b_module(void);
void register_time_module(void);

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


int main(int argc, char **argv) {

    char *source = NULL;

    if (!isatty(STDIN_FILENO)) {
        /* Read the script from stdin. */
        source = read_stream(stdin);
        if (!source) {
            fprintf(stderr, "error: cannot read stdin\n");
            return 1;
        }
    } else {
        /* Read the script from a file. */
        if (argc != 2) {
            fprintf(stderr, "usage: %s script.lx\n", argv[0]);
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
    }

    /* Initialize lexer and parser. */
    Parser parser;
    lexer_init(&parser.lexer, source);
    parser.current.type = TOK_ERROR; /* force first advance */
    parser.previous.type = TOK_ERROR;
    lx_error_clear();

    /* Parse the program. */
    AstNode *program = parse_program(&parser);
    if (lx_has_error()) {
        lx_print_error(stderr);
        free(source);
        return 1;
    }
    if (!program) {
        lx_print_error(stderr);
        free(source);
        return 1;
    }

    /* Create the global environment. */
    Env *global = env_new(NULL);

    /* Install the standard library. */
    install_stdlib();
    register_fs_module();
    register_json_module();
    register_serializer_module();
    register_hex_module();
    register_blake2b_module();
    register_time_module();
    /* Run extension initializers. */
    lx_init_modules(global);

    /* Execute. */
    EvalResult r = eval_program(program, global);
    if (lx_has_error()) {
        lx_print_error(stderr);
        value_free(r.value);
        env_free(global);
        free(source);
        return 1;
    }

    /* Cleanup. */
    value_free(r.value);
    env_free(global);
    /* Free the AST if you add an ast_free() helper. */

    free(source);
    return 0;
}
