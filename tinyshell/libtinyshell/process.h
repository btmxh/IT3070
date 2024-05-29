#pragma once

#include "parse_cmd.h"
#ifdef WIN32
#include <windows.h>
typedef PROCESS_INFORMATION process;
#else
#include <spawn.h>
typedef pid_t process;
#endif

#include "tinyshell.h"
#include <stdbool.h>

typedef struct tinyshell tinyshell;

// Win32 API passes arguments by the command line string,
// while POSIX API requires the arguments array
int process_create(process *p, char *binary_path, const tinyshell *shell,
                   const char *command, command_parse_result *parse_result,
                   char **error);
void process_free(process *p);

char *find_executable(const char *arg0, const tinyshell *shell);

// blocking
int process_wait_for(process *p, int *status_code);

// non-blocking
int process_try_wait_for(process *p, int *status_code, int *done);

int process_kill(process *p);
int process_suspend(process *p);
int process_resume(process *p);

