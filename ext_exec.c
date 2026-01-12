/**
 * @file ext_exec.c
 * @brief Exec extension module.
 */
#include "lx_ext.h"
#include "array.h"
#include <errno.h>
#include <fcntl.h>
#include <stdlib.h>
#include <string.h>
#include <sys/select.h>
#include <sys/types.h>
#include <sys/wait.h>
#include <unistd.h>

#define LX_STDIN 0
#define LX_STDOUT 1
#define LX_STDERR 2

static void array_clear(Array *a) {
    if (!a) return;
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

static int append_buf(char **buf, size_t *len, size_t *cap, const char *data, size_t n) {
    if (*len + n + 1 > *cap) {
        size_t ncap = (*cap == 0) ? 256 : *cap;
        while (*len + n + 1 > ncap) ncap *= 2;
        char *nb = (char *)realloc(*buf, ncap);
        if (!nb) return 0;
        *buf = nb;
        *cap = ncap;
    }
    memcpy(*buf + *len, data, n);
    *len += n;
    (*buf)[*len] = '\0';
    return 1;
}

static int emit_line(Array *out, const char *line, size_t len, int stream_id) {
    Value row = value_array();
    array_set(row.a, key_int(0), value_string_n(line, len));
    array_set(row.a, key_int(1), value_int(stream_id));
    lx_int_t idx = array_next_index(out);
    array_set(out, key_int(idx), row);
    return 1;
}

static void flush_buffer(Array *out, char **buf, size_t *len, int stream_id) {
    if (*len == 0) return;
    emit_line(out, *buf, *len, stream_id);
    free(*buf);
    *buf = NULL;
    *len = 0;
}

static int drain_stream(Array *out, int fd, char **buf, size_t *len, size_t *cap, int stream_id) {
    char tmp[512];
    ssize_t n = read(fd, tmp, sizeof(tmp));
    if (n <= 0) return (int)n;
    size_t start = 0;
    for (ssize_t i = 0; i < n; i++) {
        if (tmp[i] == '\n') {
            size_t seg_len = (size_t)i - start;
            if (seg_len) {
                if (!append_buf(buf, len, cap, tmp + start, seg_len)) return -1;
            }
            emit_line(out, *buf ? *buf : "", *len, stream_id);
            free(*buf);
            *buf = NULL;
            *len = 0;
            *cap = 0;
            start = (size_t)i + 1;
        }
    }
    if (start < (size_t)n) {
        if (!append_buf(buf, len, cap, tmp + start, (size_t)n - start)) return -1;
    }
    return (int)n;
}

static int run_exec(const char *cmd, Array *out) {
    int out_pipe[2];
    int err_pipe[2];
    if (pipe(out_pipe) != 0) return -1;
    if (pipe(err_pipe) != 0) { close(out_pipe[0]); close(out_pipe[1]); return -1; }

    pid_t pid = fork();
    if (pid < 0) {
        close(out_pipe[0]); close(out_pipe[1]);
        close(err_pipe[0]); close(err_pipe[1]);
        return -1;
    }
    if (pid == 0) {
        dup2(out_pipe[1], STDOUT_FILENO);
        dup2(err_pipe[1], STDERR_FILENO);
        close(out_pipe[0]);
        close(out_pipe[1]);
        close(err_pipe[0]);
        close(err_pipe[1]);
        execl("/bin/sh", "sh", "-c", cmd, (char *)NULL);
        _exit(127);
    }

    close(out_pipe[1]);
    close(err_pipe[1]);

    char *obuf = NULL;
    size_t olen = 0, ocap = 0;
    char *ebuf = NULL;
    size_t elen = 0, ecap = 0;
    int out_open = 1;
    int err_open = 1;
    int maxfd = out_pipe[0] > err_pipe[0] ? out_pipe[0] : err_pipe[0];

    while (out_open || err_open) {
        fd_set rfds;
        FD_ZERO(&rfds);
        if (out_open) FD_SET(out_pipe[0], &rfds);
        if (err_open) FD_SET(err_pipe[0], &rfds);
        int rc = select(maxfd + 1, &rfds, NULL, NULL, NULL);
        if (rc < 0) {
            if (errno == EINTR) continue;
            break;
        }
        if (out_open && FD_ISSET(out_pipe[0], &rfds)) {
            int n = drain_stream(out, out_pipe[0], &obuf, &olen, &ocap, LX_STDOUT);
            if (n == 0) {
                out_open = 0;
                close(out_pipe[0]);
                flush_buffer(out, &obuf, &olen, LX_STDOUT);
            } else if (n < 0) {
                out_open = 0;
                close(out_pipe[0]);
            }
        }
        if (err_open && FD_ISSET(err_pipe[0], &rfds)) {
            int n = drain_stream(out, err_pipe[0], &ebuf, &elen, &ecap, LX_STDERR);
            if (n == 0) {
                err_open = 0;
                close(err_pipe[0]);
                flush_buffer(out, &ebuf, &elen, LX_STDERR);
            } else if (n < 0) {
                err_open = 0;
                close(err_pipe[0]);
            }
        }
    }

    flush_buffer(out, &obuf, &olen, LX_STDOUT);
    flush_buffer(out, &ebuf, &elen, LX_STDERR);

    int status = 0;
    if (waitpid(pid, &status, 0) < 0) return -1;
    if (WIFEXITED(status)) return WEXITSTATUS(status);
    if (WIFSIGNALED(status)) return 128 + WTERMSIG(status);
    return 1;
}

static Value n_exec(Env *env, int argc, Value *argv){
    (void)env;
    if (argc < 1) return value_int(-1);
    Value cv = value_to_string(argv[0]);
    const char *cmd = cv.s ? cv.s : "";

    Array *out = NULL;
    if (argc >= 2 && argv[1].type == VAL_ARRAY && argv[1].a) {
        out = argv[1].a;
        array_clear(out);
    }

    int status = -1;
    if (out) {
        status = run_exec(cmd, out);
    } else {
        Array *tmp = array_new();
        if (tmp) {
            status = run_exec(cmd, tmp);
            array_free(tmp);
        }
    }

    value_free(cv);
    return value_int(status);
}

static void exec_module_init(Env *global){
    lx_register_function("exec", n_exec);
    (void)global;
}

void register_exec_module(void) {
    lx_register_extension("exec");
    lx_register_module(exec_module_init);
}
