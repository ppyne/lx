CC ?= gcc
CFLAGS ?= -Wall -Wextra -std=c99
LDFLAGS ?= -lm

SRCS = lexer.c parser.c main.c value.c array.c env.c natives.c eval.c gc.c lx_ext.c lx_error.c ext_fs.c ext_json.c ext_serializer.c ext_hex.c ext_blake2b.c blake2b-ref.c
OBJS = $(SRCS:.c=.o)

all: lx

lx: $(OBJS)
	$(CC) $(CFLAGS) -o $@ $(OBJS) $(LDFLAGS)

%.o: %.c
	$(CC) $(CFLAGS) -c $< -o $@

clean:
	rm -f $(OBJS) lx

test: lx
	cd tests && ./run_tests.sh

.PHONY: all clean test
