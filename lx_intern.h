/**
 * @file lx_intern.h
 * @brief String interning utilities.
 */
#ifndef LX_INTERN_H
#define LX_INTERN_H

const char *lx_intern(const char *s);
char *lx_intern_take(char *s);
int lx_intern_is_interned(const char *s);

#endif /* LX_INTERN_H */
