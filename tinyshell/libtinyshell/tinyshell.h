#pragma once

#include "process.h"
#include <stdbool.h>

typedef struct tinyshell {
  int exit;
  int has_fg;
  process fg;
} tinyshell;

int tinyshell_new(tinyshell *shell);
int tinyshell_run(tinyshell *shell);
void tinyshell_destroy(tinyshell *shell);

const char *tinyshell_get_path_env(const tinyshell *shell);

char *get_current_directory();
