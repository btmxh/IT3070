#pragma once

#include "process.h"
#include <stdio.h>

typedef struct {
  process p;
  char *cmd;
  enum { BG_PROCESS_RUNNING, BG_PROCESS_STOPPED, BG_PROCESS_EMPTY } status;
} bg_process;

typedef struct tinyshell {
  int exit;
  int has_fg;
  process fg;
  bg_process *bg;
  int bg_cap;
  char *path;
  FILE *input;
} tinyshell;

int tinyshell_new(tinyshell *shell, FILE *input);
int tinyshell_run(tinyshell *shell);
void tinyshell_destroy(tinyshell *shell);

const char *tinyshell_get_path_env(const tinyshell *shell);

char *get_current_directory();
