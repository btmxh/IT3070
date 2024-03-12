#pragma once

typedef struct {
  int argc;
  char **argv;
  int foreground;
} command_parse_result;

int parse_command(const char *command, command_parse_result *result,
                  char **error);
void command_parse_result_free(command_parse_result *result);

typedef enum {
  PARSE_ARG_NORMAL,
  PARSE_ARG_EMPTY,
  PARSE_ARG_FOREGROUND,
  PARSE_ARG_ERROR,
} parse_arg_result;

parse_arg_result parse_arg(const char **end, char **arg, char **error);
