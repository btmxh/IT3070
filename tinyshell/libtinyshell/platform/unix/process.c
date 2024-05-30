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

int process_create(process *p, char *binary_path, const tinyshell *shell,
                   const char *command, command_parse_result *parse_result,
                   char **error) {
  posix_spawn_file_actions_t fa;
  posix_spawn_file_actions_init(&fa);
  posix_spawn_file_actions_adddup2(&fa, fileno(stdin), 0);
  posix_spawn_file_actions_adddup2(&fa, fileno(stdout), 1);
  posix_spawn_file_actions_adddup2(&fa, fileno(stderr), 2);

  int error_code =
      posix_spawn(p, binary_path, &fa, NULL, parse_result->argv, NULL);
  if (error_code != 0) {
    *error = printf_to_string("%s", strerror(error_code));
  } else {
    free(binary_path);
    command_parse_result_free(parse_result);
  }

  posix_spawn_file_actions_destroy(&fa);
  return error_code == 0;
}

static int is_regular_file(const char *path) {
  struct stat s;
  return stat(path, &s) == 0 && S_ISREG(s.st_mode);
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

  char *binary_path = NULL, *saveptr;
  for (char *token = reentrant_strtok(path, ":", &saveptr); token;
       token = reentrant_strtok(NULL, ":", &saveptr)) {
    binary_path = printf_to_string("%s/%s", token, arg0);
    if (binary_path != NULL && check_executable(binary_path)) {
      break;
    }

    free(binary_path);
    binary_path = NULL;
  }

  free(path);
  return binary_path;
}

static char *find_executable_slash(const char *arg0, const tinyshell *shell) {
  if (!check_executable(arg0)) {
    return NULL;
  }

  return printf_to_string("%s", arg0);
}

char *find_executable(const char *arg0, const tinyshell *shell) {
  if (strchr(arg0, '/') == NULL) {
    return find_executable_no_slash(arg0, shell);
  } else {
    return find_executable_slash(arg0, shell);
  }
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

int process_kill(process *p) { return kill(*p, SIGINT) != -1; }

int process_suspend(process *p) { return kill(*p, SIGSTOP) != -1; }

int process_resume(process *p) { return kill(*p, SIGCONT) != -1; }
