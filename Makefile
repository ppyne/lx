CC ?= gcc
CFLAGS ?= -Wall -Wextra -std=c99 -O2
LDFLAGS ?= -lm
NO_VERSION ?= 0
CONFIG_H ?= config.h

BASE_SRCS = lexer.c parser.c main.c value.c array.c env.c natives.c eval.c gc.c lx_ext.c lx_error.c
EXT_SRCS =
LX_ENABLE_FS := $(shell awk '/^\#define[ \t]+LX_ENABLE_FS/{print $$3}' $(CONFIG_H) 2>/dev/null)
LX_ENABLE_JSON := $(shell awk '/^\#define[ \t]+LX_ENABLE_JSON/{print $$3}' $(CONFIG_H) 2>/dev/null)
LX_ENABLE_SERIALIZER := $(shell awk '/^\#define[ \t]+LX_ENABLE_SERIALIZER/{print $$3}' $(CONFIG_H) 2>/dev/null)
LX_ENABLE_HEX := $(shell awk '/^\#define[ \t]+LX_ENABLE_HEX/{print $$3}' $(CONFIG_H) 2>/dev/null)
LX_ENABLE_BLAKE2B := $(shell awk '/^\#define[ \t]+LX_ENABLE_BLAKE2B/{print $$3}' $(CONFIG_H) 2>/dev/null)
LX_ENABLE_TIME := $(shell awk '/^\#define[ \t]+LX_ENABLE_TIME/{print $$3}' $(CONFIG_H) 2>/dev/null)
LX_ENABLE_ENV := $(shell awk '/^\#define[ \t]+LX_ENABLE_ENV/{print $$3}' $(CONFIG_H) 2>/dev/null)
LX_ENABLE_UTF8 := $(shell awk '/^\#define[ \t]+LX_ENABLE_UTF8/{print $$3}' $(CONFIG_H) 2>/dev/null)
LX_ENABLE_SQLITE := $(shell awk '/^\#define[ \t]+LX_ENABLE_SQLITE/{print $$3}' $(CONFIG_H) 2>/dev/null)
LX_ENABLE_INCLUDE := $(shell awk '/^\#define[ \t]+LX_ENABLE_INCLUDE/{print $$3}' $(CONFIG_H) 2>/dev/null)
LX_ENABLE_AEAD := $(shell awk '/^\#define[ \t]+LX_ENABLE_AEAD/{print $$3}' $(CONFIG_H) 2>/dev/null)
LX_ENABLE_ED25519 := $(shell awk '/^\#define[ \t]+LX_ENABLE_ED25519/{print $$3}' $(CONFIG_H) 2>/dev/null)
LX_ENABLE_EXEC := $(shell awk '/^\#define[ \t]+LX_ENABLE_EXEC/{print $$3}' $(CONFIG_H) 2>/dev/null)

ifneq ($(LX_ENABLE_FS),0)
EXT_SRCS += ext_fs.c
endif
ifneq ($(LX_ENABLE_JSON),0)
EXT_SRCS += ext_json.c
endif
ifneq ($(LX_ENABLE_SERIALIZER),0)
EXT_SRCS += ext_serializer.c
endif
ifneq ($(LX_ENABLE_HEX),0)
EXT_SRCS += ext_hex.c
endif
ifneq ($(LX_ENABLE_BLAKE2B),0)
EXT_SRCS += ext_blake2b.c blake2b-ref.c
endif
ifneq ($(LX_ENABLE_TIME),0)
EXT_SRCS += ext_time.c
endif
ifneq ($(LX_ENABLE_ENV),0)
EXT_SRCS += ext_env.c
endif
ifneq ($(LX_ENABLE_UTF8),0)
EXT_SRCS += ext_utf8.c
endif
ifneq ($(LX_ENABLE_SQLITE),0)
EXT_SRCS += ext_sqlite.c
LDFLAGS += -lsqlite3
endif
ifneq ($(LX_ENABLE_AEAD),0)
EXT_SRCS += ext_aead.c
MONO_SRCS = monocypher.c
endif
ifneq ($(LX_ENABLE_ED25519),0)
EXT_SRCS += ext_ed25519.c
MONO_SRCS = monocypher.c
endif
ifneq ($(LX_ENABLE_EXEC),0)
EXT_SRCS += ext_exec.c
endif
ifneq ($(MONO_SRCS),)
EXT_SRCS += $(MONO_SRCS)
endif

SRCS = $(BASE_SRCS) $(EXT_SRCS)
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

%.o: %.c $(VERSION_HEADER) $(CONFIG_H)
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
