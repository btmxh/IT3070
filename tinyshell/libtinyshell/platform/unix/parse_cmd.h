#pragma once

#include "../../process.h"

typedef struct {
  int argc;
  char **argv;
  int foreground;
} command_parse_result;

int parse_command(const char *command,
                                command_parse_result *result, char **error);
void command_parse_result_free(command_parse_result *result);
