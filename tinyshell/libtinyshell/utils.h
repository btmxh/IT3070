#pragma once

#include <assert.h>
#include <stdarg.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

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

inline static int vecpush(void **dest, int *len, int *cap, int elemsize,
                   const void *src, int srclen) {
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

  memcpy((char*)*dest + *len * elemsize, src, srclen * elemsize);
  *len += srclen;
  return 1;
}

