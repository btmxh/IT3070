#include "tinyshell.h"
#include <stdio.h>

int tinyshell_new(tinyshell* shell) {
  return 1;
}

int tinyshell_run(tinyshell* shell) {
  printf("Hello, World!\n");
  return 1;
}

void tinyshell_destroy(tinyshell* shell) {
}

const char* tinyshell_get_current_directory(const tinyshell* shell) {
  return ".";
}

const char* tinyshell_get_path_env(const tinyshell* shell) {
  return "";
}
