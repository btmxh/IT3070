#pragma once

#ifdef WIN32
#include <Windows.h>
typedef PROCESS_INFORMATION process;
#else
#include <spawn.h>
typedef pid_t process;
#endif

#include "tinyshell.h"

typedef enum {
  PROCESS_CREATE_SUCCESS = 0,
  PROCESS_CREATE_ERROR_EMPTY_COMMAND,
  PROCESS_CREATE_ERROR_INVALID_COMMAND,
  PROCESS_CREATE_ERROR_OUT_OF_MEMORY,
  PROCESS_CREATE_ERROR_UNABLE_TO_SPAWN_PROCESS,
} process_create_error;

process_create_error process_create(process *p, const tinyshell *shell,
                                    const char *command, char **error);
void process_free(process *p);

// blocking
int process_wait_for(process *p, int *status_code);

// non-blocking
int process_try_wait_for(process *p, int *status_code, int* done);

int process_kill(process *p);
int process_suspend(process *p);
int process_resume(process *p);
