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
