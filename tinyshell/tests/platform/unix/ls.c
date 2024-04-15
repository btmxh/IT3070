#include "parse_cmd.h"
#include "process.h"
#include <assert.h>
#include <stdlib.h>
#include <unistd.h>
#include <string.h>

int main() {
  tinyshell shell;
  process p;
  command_parse_result cpr;
  cpr.argc = 2;
  cpr.argv = malloc(3 * sizeof(char*));
  cpr.argv[0] = strdup("/bin/ls");
  cpr.argv[1] = strdup("-la");
  cpr.argv[2] = NULL;
  cpr.foreground = 0;
  char* error;
  int status = process_create(&p, strdup("/bin/ls"), &shell, "/bin/ls -la", &cpr, &error);
  assert(status);
  int code = 0;
  status = process_wait_for(&p, &code);
  assert(status && code == 0);
  process_free(&p);
  return 0;
}
