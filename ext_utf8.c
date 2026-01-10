/**
 * @file ext_utf8.c
 * @brief UTF-8 helpers.
 */
#include "lx_ext.h"
#include "value.h"
#include <string.h>
#include <stdint.h>

static int utf8_decode_next(const unsigned char *s, size_t len, size_t *i, uint32_t *cp) {
    if (*i >= len) return 0;
    unsigned char c0 = s[*i];
    if (c0 < 0x80) {
        *cp = c0;
        (*i)++;
        return 1;
    }

    if (c0 >= 0xC2 && c0 <= 0xDF) {
        if (*i + 1 >= len) return 0;
        unsigned char c1 = s[*i + 1];
        if ((c1 & 0xC0) != 0x80) return 0;
        *cp = ((uint32_t)(c0 & 0x1F) << 6) | (uint32_t)(c1 & 0x3F);
        *i += 2;
        return 1;
    }

    if (c0 >= 0xE0 && c0 <= 0xEF) {
        if (*i + 2 >= len) return 0;
        unsigned char c1 = s[*i + 1];
        unsigned char c2 = s[*i + 2];
        if ((c1 & 0xC0) != 0x80 || (c2 & 0xC0) != 0x80) return 0;
        if (c0 == 0xE0 && c1 < 0xA0) return 0;
        if (c0 == 0xED && c1 >= 0xA0) return 0;
        *cp = ((uint32_t)(c0 & 0x0F) << 12) |
              ((uint32_t)(c1 & 0x3F) << 6) |
              (uint32_t)(c2 & 0x3F);
        *i += 3;
        return 1;
    }

    if (c0 >= 0xF0 && c0 <= 0xF4) {
        if (*i + 3 >= len) return 0;
        unsigned char c1 = s[*i + 1];
        unsigned char c2 = s[*i + 2];
        unsigned char c3 = s[*i + 3];
        if ((c1 & 0xC0) != 0x80 || (c2 & 0xC0) != 0x80 || (c3 & 0xC0) != 0x80) return 0;
        if (c0 == 0xF0 && c1 < 0x90) return 0;
        if (c0 == 0xF4 && c1 > 0x8F) return 0;
        *cp = ((uint32_t)(c0 & 0x07) << 18) |
              ((uint32_t)(c1 & 0x3F) << 12) |
              ((uint32_t)(c2 & 0x3F) << 6) |
              (uint32_t)(c3 & 0x3F);
        *i += 4;
        return 1;
    }

    return 0;
}

static Value n_glyph_count(Env *env, int argc, Value *argv){
    (void)env;
    if (argc != 1 || argv[0].type != VAL_STRING) return value_undefined();
    const unsigned char *s = (const unsigned char *)(argv[0].s ? argv[0].s : "");
    size_t len = strlen((const char *)s);
    size_t i = 0;
    int count = 0;
    uint32_t cp = 0;
    while (i < len) {
        if (!utf8_decode_next(s, len, &i, &cp)) return value_undefined();
        count++;
    }
    return value_int(count);
}

static Value n_glyph_at(Env *env, int argc, Value *argv){
    (void)env;
    if (argc != 2 || argv[0].type != VAL_STRING) return value_undefined();
    int idx = value_to_int(argv[1]).i;
    if (idx < 0) return value_undefined();
    const unsigned char *s = (const unsigned char *)(argv[0].s ? argv[0].s : "");
    size_t len = strlen((const char *)s);
    size_t i = 0;
    int pos = 0;
    uint32_t cp = 0;
    while (i < len) {
        size_t start = i;
        if (!utf8_decode_next(s, len, &i, &cp)) return value_undefined();
        if (pos == idx) {
            return value_string_n((const char *)s + start, i - start);
        }
        pos++;
    }
    return value_undefined();
}

static void utf8_module_init(Env *global){
    lx_register_function("glyph_count", n_glyph_count);
    lx_register_function("glyph_at", n_glyph_at);
    (void)global;
}

void register_utf8_module(void) {
    lx_register_extension("utf8");
    lx_register_module(utf8_module_init);
}
