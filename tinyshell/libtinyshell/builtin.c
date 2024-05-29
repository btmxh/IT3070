#include "builtin.h"
#include "parse_cmd.h"
#include "tinyshell.h"
#include "utils.h"

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#ifdef _WIN32
#include <direct.h>
#else
#include <unistd.h>
#endif

int try_run_builtin(tinyshell *shell, command_parse_result *result,
                    int *status_code) {
  const char *arg0 = result->argv[0];
  if (strcmp(arg0, "cd") == 0) {
    *status_code = builtin_cd(shell, result->argc, result->argv);
    command_parse_result_free(result);
    return 1;
  } else if (strcmp(arg0, "pwd") == 0) {
    *status_code = builtin_pwd(shell, result->argc, result->argv);
    command_parse_result_free(result);
    return 1;
  }

  return 0;
}

int builtin_cd(tinyshell *shell, int argc, char *argv[]) {
  if (argc != 2) {
    puts("usage: cd <directory>");
    return 1;
  }

  return chdir(argv[1]);
}

int builtin_pwd(tinyshell *shell, int argc, char *argv[]) {
  char* cwd = get_current_directory();
  if(!cwd) {
    return 1;
  }

  puts(cwd);
  free(cwd);
  return 0;
}
