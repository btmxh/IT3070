#pragma once

#include <assert.h>
#include <stdarg.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#define CONCAT(x, y) x##y
#ifdef WIN32
#define POSIX_WIN32(func) CONCAT(_, func)
#else
#define POSIX_WIN32(func) func
#endif

inline static int max_int(int x, int y) { return x > y ? x : y; }

inline static char *printf_to_string(const char *fmt, ...) {
  va_list va;
  va_start(va, fmt);
  int length = vsnprintf(NULL, 0, fmt, va);
  va_end(va);

  char *string = calloc(length + 1, 1);
  if (string == NULL) {
    return NULL;
  }

  va_start(va, fmt);
  vsnprintf(string, length + 1, fmt, va);
  va_end(va);

  return string;
}

inline static int vecpush(void * /* T** */ dst, int *len, int *cap,
                          int elemsize, const void *src, int srclen) {
  void **dest = (void **)dst;
  const float scale_factor = 1.5;
  if (*len + srclen >= *cap) {
    int new_cap = max_int(*cap * scale_factor, *len + srclen);
    void *new_dest = realloc(*dest, new_cap * elemsize);
    if (!new_dest) {
      return 0;
    }

    *dest = new_dest;
    *cap = new_cap;
  }

  memcpy((char *)*dest + *len * elemsize, src, srclen * elemsize);
  *len += srclen;
  return 1;
}

// reentrant version of strtok, reimplemented here for portability
// source: https://git.musl-libc.org/cgit/musl/tree/src/string/strtok_r.c
// (C11 restrict removed, renamed to avoid collision)
inline static char *reentrant_strtok(char *s, const char *sep, char **p) {
  if (!s && !(s = *p))
    return NULL;
  s += strspn(s, sep);
  if (!*s)
    return *p = 0;
  *p = s + strcspn(s, sep);
  if (**p)
    *(*p)++ = 0;
  else
    *p = 0;
  return s;
}
