#include "process.h"
#include "parse_cmd.h"
#include "tinyshell.h"
#include "utils.h"

#include <assert.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include <signal.h>
#include <spawn.h>
#include <sys/stat.h>
#include <sys/wait.h>
#include <unistd.h>

static int call_posix_spawn(process *p, const char *binary_path,
                            command_parse_result *result, char **error) {
  posix_spawn_file_actions_t fa;
  posix_spawn_file_actions_init(&fa);
  posix_spawn_file_actions_adddup2(&fa, fileno(stdin), 0);
  posix_spawn_file_actions_adddup2(&fa, fileno(stdout), 1);
  posix_spawn_file_actions_adddup2(&fa, fileno(stderr), 2);

  int error_code = posix_spawn(p, binary_path, &fa, NULL, result->argv, NULL);
  if (error_code == 0) {
    posix_spawn_file_actions_destroy(&fa);
    return 1;
  }

  *error = printf_to_string("%s", strerror(error_code));
  posix_spawn_file_actions_destroy(&fa);
  return 0;
}

static int is_regular_file(const char *path) {
  struct stat s;
  stat(path, &s);
  return S_ISREG(s.st_mode);
}

static int check_executable(const char *path) {
  return is_regular_file(path) && access(path, X_OK) == 0;
}

static char *find_executable_no_slash(const char *arg0,
                                      const tinyshell *shell) {
  // search in path directories
  char *path = printf_to_string("%s", tinyshell_get_path_env(shell));
  if (path == NULL) {
    return NULL;
  }

  char *token = strtok(path, ":");
  char *binary_path = NULL;
  while (token != NULL) {
    binary_path = printf_to_string("%s/%s", token, arg0);
    if (binary_path != NULL && check_executable(binary_path)) {
      break;
    }
    token = strtok(NULL, ":");
    free(binary_path);
    binary_path = NULL;
  }

  free(path);
  return binary_path;
}

static char *find_executable_slash(const char *arg0, const tinyshell *shell) {
  char *path;
  if (arg0[0] == '/') {
    // absolute path
    path = printf_to_string("%s", arg0);
  } else {
    // relative path
    path = printf_to_string("%s/%s", tinyshell_get_current_directory(shell), arg0);
  }

  if(path != NULL && !check_executable(path)) {
    path = NULL;
  }

  return path;
}

static char *find_executable(const char *arg0, const tinyshell *shell) {
  if (strchr(arg0, '/') == NULL) {
    return find_executable_no_slash(arg0, shell);
  } else {
    return find_executable_slash(arg0, shell);
  }
}

process_create_error process_create(process *p, const tinyshell *shell,
                                    const char *command, char **error,
                                    int *foreground) {
  int retval;
  *error = NULL;
  command_parse_result parse_result;
  if (!parse_command(command, &parse_result, error)) {
    return PROCESS_CREATE_ERROR_INVALID_COMMAND;
  }

  *foreground = parse_result.foreground;

  if (parse_result.argc == 0) {
    return PROCESS_CREATE_ERROR_EMPTY_COMMAND;
  }

  assert(parse_result.argc >= 1);
  const char *arg0 = parse_result.argv[0];

  char* executable = find_executable(arg0, shell);
  if(executable) {
    retval = call_posix_spawn(p, executable, &parse_result, error);
    retval = PROCESS_CREATE_SUCCESS;
  } else {
    retval = PROCESS_CREATE_ERROR_UNABLE_TO_SPAWN_PROCESS;
  }

  free(executable);
  command_parse_result_free(&parse_result);
  return retval;
}

void process_free(process *p) {}

// blocking
int process_wait_for(process *p, int *status_code) {
  int wstatus;
  if (waitpid(*p, &wstatus, 0) == -1) {
    perror("waitpid");
    return 0;
  }

  if (status_code) {
    *status_code = WEXITSTATUS(wstatus);
  }
  return 1;
}

// non-blocking
int process_try_wait_for(process *p, int *status_code, int *done) {
  int wstatus;
  if (waitpid(*p, &wstatus, WNOHANG) == -1) {
    perror("waitpid");
    return 0;
  }

  if (status_code) {
    *status_code = WEXITSTATUS(wstatus);
  }

  if (done) {
    *done = WIFEXITED(wstatus);
  }

  return 1;
}

int process_kill(process *p) {
  return kill(*p, SIGKILL) != -1 && process_wait_for(p, NULL);
}

int process_suspend(process *p) { return kill(*p, SIGSTOP) != -1; }

int process_resume(process *p) { return kill(*p, SIGCONT) != -1; }
