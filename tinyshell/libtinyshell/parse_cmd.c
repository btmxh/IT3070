#include "parse_cmd.h"
#include <assert.h>
#include <ctype.h>
#include <stdarg.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#ifdef WIN32
#define ESCAPE_CHAR '^'
#define IS_QUOTE(c) ((c) == '"')
#else
#define ESCAPE_CHAR '\\'
#define IS_QUOTE(c) ((c) == '"' || (c) == '\'')
#endif

static int max_int(int x, int y) { return x > y ? x : y; }

static char *printf_to_string(const char *fmt, ...) {
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

typedef enum {
  PARSE_CODEPOINT_NORMAL,
  PARSE_CODEPOINT_QUOTE,
  PARSE_CODEPOINT_SPACE,
  PARSE_CODEPOINT_NULL_TERM,
  PARSE_CODEPOINT_ERROR,
  PARSE_CODEPOINT_AMPERSAND
} parse_codepoint_result;

static int vecpush(void **dest, int *len, int *cap, int elemsize,
                   const void *src, int srclen) {
  const float scale_factor = 1.5;
  if (*len + srclen >= *cap) {
    int new_cap = max_int((int)(*cap * scale_factor), *len + srclen);
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

static parse_codepoint_result parse_next_codepoint(const char **end, char* quote,
                                                   char cp[8], char **error) {
  if (**end == '\0') {
    ++*end;
    return PARSE_CODEPOINT_NULL_TERM;
  }

  if(**end == '&') {
    ++*end;
    return PARSE_CODEPOINT_AMPERSAND;
  }

  if (**end == ESCAPE_CHAR) {
    ++*end;
    if (**end == '\0') {
      *error = printf_to_string("incomplete escape token");
      return PARSE_CODEPOINT_ERROR;
    }

    // TODO: support unicode escape?
    cp[0] = **end;
    cp[1] = '\0';
    ++*end;
    return PARSE_CODEPOINT_NORMAL;
  }

  if(IS_QUOTE(**end)) {
    if(*quote == '\0') {
      *quote = **end;
      ++*end;
      return PARSE_CODEPOINT_QUOTE;
    } else if(*quote == **end) {
      *quote = '\0';
      ++*end;
      return PARSE_CODEPOINT_QUOTE;
    }
  }

  cp[0] = **end;
  cp[1] = '\0';
  ++*end;
  if (isspace(cp[0]) && *quote == '\0') {
    return PARSE_CODEPOINT_SPACE;
  }

  return PARSE_CODEPOINT_NORMAL;
}

parse_arg_result parse_arg(const char **end, char **arg, char **error) {
  while (isspace(**end) && **end != '\0')
    ++*end;

  if (**end == '\0') {
    *arg = NULL;
    ++*end;
    return PARSE_ARG_EMPTY;
  }

  if(**end == '&') {
    *arg = NULL;
    ++*end;
    return PARSE_ARG_BACKGROUND;
  }

  char quote = '\0';
  *arg = NULL;
  int arg_cap = 0;
  int arg_len = 0;
  while (**end != '\0') {
    char c[8];
    parse_codepoint_result typ = parse_next_codepoint(end, &quote, c, error);
    switch (typ) {
    case PARSE_CODEPOINT_NORMAL: {
      int n = (int)strlen(c); // currently n == 1
      if (!vecpush((void **)arg, &arg_len, &arg_cap, 1, c, strlen(c))) {
        *error = printf_to_string("unable to allocate memory for arg");
        goto fail_realloc_arg;
      }
      break;
    }
    case PARSE_CODEPOINT_QUOTE:
      break;
    case PARSE_CODEPOINT_NULL_TERM:
    case PARSE_CODEPOINT_SPACE:
      goto outer;
    case PARSE_CODEPOINT_ERROR:
      goto fail_parse_codepoints;
    case PARSE_CODEPOINT_AMPERSAND:
      --*end;
      goto outer;
    }
  }
outer:
  if (quote != '\0') {
    *error = printf_to_string("unclosed quotes %c", quote);
    goto fail_unclosed_quotes;
  }

  char nullterm = '\0';
  if (!vecpush((void **)arg, &arg_len, &arg_cap, 1, &nullterm, 1)) {
    *error = printf_to_string(
        "unable to allocate memory to null-terminate argument string");
    goto fail_realloc_arg;
  }

  return PARSE_ARG_NORMAL;

fail_unclosed_quotes:
fail_parse_codepoints:
fail_realloc_arg:
  free(*arg);
  return PARSE_ARG_ERROR;
}

int parse_command(const char *command, command_parse_result *result,
                  char **error) {
#define ARGV_SCALE_FACTOR 2
  result->argv = NULL;
  result->argc = 0;
  result->foreground = 1;
  int argv_cap = 0;
  while (1) {
    char *arg;
    parse_arg_result arg_result = parse_arg(&command, &arg, error);
    if (!result->foreground && arg_result != PARSE_ARG_EMPTY) {
      *error = printf_to_string(
          "& (background specifier) should be the last arg in command, as this "
          "shell does not support composite commands on Unix");
      goto fail_parse_arg;
    }
    switch (arg_result) {
    case PARSE_ARG_NORMAL:
      if (!vecpush((void **)&result->argv, &result->argc, &argv_cap,
                   sizeof(char *), &arg, 1)) {
        printf_to_string("unable to allocate memory for argv");
        goto fail_realloc_arg;
      }
      break;
    case PARSE_ARG_EMPTY:
      goto outer;
    case PARSE_ARG_BACKGROUND:
      result->foreground = 0;
      break;
    case PARSE_ARG_ERROR:
      goto fail_parse_arg;
      break;
    }
  }
outer:;
  char *nullterm = NULL;
  if (!vecpush((void **)&result->argv, &result->argc, &argv_cap, sizeof(char *),
               &nullterm, 1)) {
    printf_to_string(
        "unable to allocate memory to null-terminate argument array");
    goto fail_realloc_arg;
  }

  return 1;

fail_parse_arg:
fail_realloc_arg:
  for (int i = 0; i < result->argc; ++i) {
    free(result->argv[i]);
  }
  free(result->argv);
  return 0;
}

void command_parse_result_free(command_parse_result *result) {
  for (int i = 0; i < result->argc - 1; ++i) {
    free(result->argv[i]);
  }
  free(result->argv);
}
