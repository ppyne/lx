/**
 * @file main_runner.c
 * @brief Command-line entry point for running scripts.
 */

#include <stdio.h>
#include <stdlib.h>

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

/* File reading utility. */
static char *read_file(const char *path)
{
    FILE *f = fopen(path, "rb");
    if (!f) return NULL;

    fseek(f, 0, SEEK_END);
    long size = ftell(f);
    rewind(f);

    char *buf = (char *)malloc(size + 1);
    if (!buf) {
        fclose(f);
        return NULL;
    }

    fread(buf, 1, size, f);
    buf[size] = 0;
    fclose(f);
    return buf;
}

int main(int argc, char **argv)
{
    if (argc != 2) {
        fprintf(stderr, "usage: %s script.lx\n", argv[0]);
        return 1;
    }

    /* Read the script. */
    char *source = read_file(argv[1]);
    if (!source) {
        fprintf(stderr, "error: cannot read file '%s'\n", argv[1]);
        return 1;
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
