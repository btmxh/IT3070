#pragma once

#include "tinyshell.h"
#include <assert.h>
#include <stdio.h>

#ifdef WIN32
#define SCRIPT_EXTENSION ".tbat"
#else
#define SCRIPT_EXTENSION ".tsh"
#endif

void run_script(const char *path) {
  FILE *f = fopen(path, "r");
  tinyshell shell;
  int r = tinyshell_new(&shell, f) && tinyshell_run(&shell);
  assert(r);
  tinyshell_destroy(&shell);
  fclose(f);
}
