CC ?= gcc
CFLAGS ?= -Wall -Wextra -std=c99 -O2
LDFLAGS ?= -lm -lsqlite3
NO_VERSION ?= 0

SRCS = lexer.c parser.c main.c value.c array.c env.c natives.c eval.c gc.c lx_ext.c lx_error.c ext_fs.c ext_json.c ext_serializer.c ext_hex.c ext_blake2b.c ext_time.c ext_env.c ext_utf8.c ext_sqlite.c blake2b-ref.c
OBJS = $(SRCS:.c=.o)
CORE_OBJS = $(filter-out main.o,$(OBJS))
CGI_OBJS = lx_cgi.o
VERSION_FILE = lx_version.build
VERSION_HEADER = lx_version_build.h

all: lx

ifeq ($(NO_VERSION),1)
LX_DEPS =
else
LX_DEPS = version
endif

lx: $(LX_DEPS) $(OBJS)
	$(CC) $(CFLAGS) -o $@ $(OBJS) $(LDFLAGS)

lx_cgi: $(LX_DEPS) $(CORE_OBJS) $(CGI_OBJS)
	$(CC) $(CFLAGS) -o $@ $(CORE_OBJS) $(CGI_OBJS) $(LDFLAGS)

%.o: %.c $(VERSION_HEADER)
	$(CC) $(CFLAGS) -c $< -o $@

version: $(VERSION_FILE)
	@command -v awk >/dev/null 2>&1 && command -v git >/dev/null 2>&1 || exit 0; \
	major=$$(awk '$$2=="LX_VERSION_MAJOR"{print $$3}' lx_version.h); \
	minor=$$(awk '$$2=="LX_VERSION_MINOR"{print $$3}' lx_version.h); \
	branch=$$(git rev-parse --abbrev-ref HEAD 2>/dev/null || echo ""); \
	if [ -f $(VERSION_FILE) ]; then \
		read fmaj fmin fbuild < $(VERSION_FILE); \
	else \
		fmaj=""; fmin=""; fbuild=""; \
	fi; \
	if [ "$$branch" = "dev" ]; then \
		if [ "$$fmaj" != "$$major" ] || [ "$$fmin" != "$$minor" ] || [ -z "$$fbuild" ]; then \
			build=0; \
		else \
			build=$$((fbuild + 1)); \
		fi; \
	else \
		if [ "$$fmaj" != "$$major" ] || [ "$$fmin" != "$$minor" ] || [ -z "$$fbuild" ]; then \
			build=0; \
		else \
			build=$$fbuild; \
		fi; \
	fi; \
	echo "$$major $$minor $$build" > $(VERSION_FILE); \
	printf "/* Auto-generated. Do not edit. */\n#ifndef LX_VERSION_BUILD\n#define LX_VERSION_BUILD %s\n#endif\n" "$$build" > $(VERSION_HEADER)

clean:
	rm -f $(OBJS) $(CGI_OBJS) lx lx_cgi

test:
	$(MAKE) NO_VERSION=1 lx
	cd tests && ./run_tests.sh

.PHONY: all clean test version
