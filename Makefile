CC ?= gcc
CFLAGS ?= -Wall -Wextra -std=c99 -O2
LDFLAGS ?= -lm -lsqlite3

SRCS = lexer.c parser.c main.c value.c array.c env.c natives.c eval.c gc.c lx_ext.c lx_error.c ext_fs.c ext_json.c ext_serializer.c ext_hex.c ext_blake2b.c ext_time.c ext_env.c ext_utf8.c ext_sqlite.c blake2b-ref.c
OBJS = $(SRCS:.c=.o)
CORE_OBJS = $(filter-out main.o,$(OBJS))
CGI_OBJS = lx_cgi.o

all: lx

lx: $(OBJS)
	$(CC) $(CFLAGS) -o $@ $(OBJS) $(LDFLAGS)

lx_cgi: $(CORE_OBJS) $(CGI_OBJS)
	$(CC) $(CFLAGS) -o $@ $(CORE_OBJS) $(CGI_OBJS) $(LDFLAGS)

%.o: %.c
	$(CC) $(CFLAGS) -c $< -o $@

clean:
	rm -f $(OBJS) $(CGI_OBJS) lx lx_cgi

test: lx
	cd tests && ./run_tests.sh

.PHONY: all clean test
