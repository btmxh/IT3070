#include "process.h"
#include <assert.h>
#include <stdlib.h>

int main() {
  tinyshell shell;
  process p;
  char* error;
  int foreground;
  process_create_error status = process_create(&p, &shell, "/bin/ls -la", &error, &foreground);
#ifdef WIN32
  assert(status);
  assert(!error);
  free(error);
#else
  assert(status == PROCESS_CREATE_SUCCESS);
  int code = 0;
  status = process_wait_for(&p, &code);
  assert(code == 0);
  process_free(&p);
#endif
  return 0;
}
