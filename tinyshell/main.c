#include <stdio.h>

#include "tinyshell.h"

int main() {
  tinyshell shell;
  if(!tinyshell_new(&shell)) {
    printf("unable to initialize tinyshell\n");
  }

  tinyshell_run(&shell);
  tinyshell_destroy(&shell);

  return 0;
}
