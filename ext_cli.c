/**
 * @file ext_cli.c
 * @brief CLI-only input helpers.
 */
#include "lx_ext.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <termios.h>
#include <unistd.h>

static int append_char(char **buf, size_t *len, size_t *cap, char c) {
    if (*len + 2 > *cap) {
        size_t ncap = (*cap == 0) ? 128 : (*cap * 2);
        char *nb = (char *)realloc(*buf, ncap);
        if (!nb) return 0;
        *buf = nb;
        *cap = ncap;
    }
    (*buf)[(*len)++] = c;
    (*buf)[*len] = '\0';
    return 1;
}

static char *read_line_stdin(void) {
    char *buf = NULL;
    size_t len = 0;
    size_t cap = 0;
    int c = 0;

    while ((c = fgetc(stdin)) != EOF) {
        if (c == '\n') break;
        if (!append_char(&buf, &len, &cap, (char)c)) {
            free(buf);
            return NULL;
        }
    }

    if (c == EOF && len == 0) {
        free(buf);
        return NULL;
    }

    if (len > 0 && buf[len - 1] == '\r') {
        buf[--len] = '\0';
    }

    if (!buf) return strdup("");
    return buf;
}

static void print_prompt(Value prompt) {
    Value pv = value_to_string(prompt);
    fputs(pv.s ? pv.s : "", stdout);
    fflush(stdout);
    value_free(pv);
}

static Value n_read_line(Env *env, int argc, Value *argv){
    (void)env;
    if (argc > 1) return value_undefined();
    if (argc == 1) print_prompt(argv[0]);

    char *line = read_line_stdin();
    if (!line) return value_undefined();
    Value out = value_string(line);
    free(line);
    return out;
}

static Value n_read_key(Env *env, int argc, Value *argv){
    (void)env;
    if (argc > 1) return value_undefined();
    if (argc == 1) print_prompt(argv[0]);

    unsigned char ch = 0;
    struct termios oldt;
    int have_term = 0;
    int fd = STDIN_FILENO;

    if (isatty(fd) && tcgetattr(fd, &oldt) == 0) {
        struct termios raw = oldt;
        raw.c_lflag &= (tcflag_t)~(ICANON | ECHO);
        raw.c_cc[VMIN] = 1;
        raw.c_cc[VTIME] = 0;
        if (tcsetattr(fd, TCSANOW, &raw) == 0) {
            have_term = 1;
        }
    }

    ssize_t n = read(fd, &ch, 1);
    if (have_term) tcsetattr(fd, TCSANOW, &oldt);
    if (n <= 0) return value_undefined();
    return value_int((int)ch);
}

static void cli_module_init(Env *global){
    lx_register_function("read_line", n_read_line);
    lx_register_function("read_key", n_read_key);
    (void)global;
}

void register_cli_module(void) {
    lx_register_extension("cli");
    lx_register_module(cli_module_init);
}
