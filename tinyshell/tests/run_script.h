#pragma once

#include "tinyshell.h"
#include "utils.h"
#include <assert.h>
#include <stdio.h>

#ifdef WIN32
#include <direct.h>
#define SCRIPT_EXTENSION ".tbat"
#else
#include <unistd.h>
#define SCRIPT_EXTENSION ".tsh"
#endif

void run_script(const char *path) {
  FILE *f = fopen(path, "r");
  POSIX_WIN32(chdir)(ROOT_TEST);
  puts(path);
  assert(f);
  tinyshell shell;
  int r = tinyshell_new(&shell, f) && tinyshell_run(&shell);
  assert(r);
  tinyshell_destroy(&shell);
  fclose(f);
}
