#include "process.h"
#include <assert.h>

int main() {
  tinyshell shell;
  process p;
  char* error;
  int status = process_create(&p, &shell, "/usr/bin/ls -la", &error);
#ifdef WIN32
  assert(status);
  assert(!error);
  free(error);
#else
  assert(status == 0);
  int code = 0;
  status = process_wait_for(&p, &code);
  assert(code == 0);
#endif
  return 0;
}
