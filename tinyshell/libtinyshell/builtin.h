#include "parse_cmd.h"
#include "tinyshell.h"

int try_run_builtin(tinyshell *shell, command_parse_result *result,
                    int *status_code);

int builtin_cd(tinyshell* shell, int argc, char* argv[]);
int builtin_pwd(tinyshell* shell, int argc, char* argv[]);
