#include "process.h"
#include "parse_cmd.h"
#include "tinyshell.h"
#include "utils.h"

#include <assert.h>
#include <signal.h>
#include <spawn.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
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

  *error = strdup(strerror(error_code));
  posix_spawn_file_actions_destroy(&fa);
  return 0;
}

int process_create(process *p, const tinyshell *shell, const char *command,
                   char **error) {
  *error = NULL;

  command_parse_result parse_result;
  if (!parse_command(command, &parse_result, error)) {
    return PROCESS_CREATE_ERROR_INVALID_COMMAND;
  }

  if (parse_result.argc == 0) {
    return PROCESS_CREATE_ERROR_EMPTY_COMMAND;
  }

  assert(parse_result.argc >= 1);
  const char *arg0 = parse_result.argv[0];
  size_t arg0_len = strlen(arg0);
  if (strchr(arg0, '/') == NULL) {
    // built-in command or binary in PATH

    // TODO: add built-in commands
    const char *path = tinyshell_get_path_env(shell);
    while (*path != '\0') {
      const char *path_end = strchr(path, ':');
      if (path_end == NULL) {
        path_end = path + strlen(path);
      }

      size_t cur_path_len = (size_t)(path_end - path);
      char *binary_path = malloc((size_t)(path_end - path) + arg0_len + 2);
      if (!binary_path) {
        goto fail_alloc_binary_path;
      }
      memcpy(binary_path, path, cur_path_len);
      binary_path[cur_path_len] = '/';
      memcpy(&binary_path[cur_path_len + 1], arg0, arg0_len + 1);

      if (access(binary_path, X_OK) == 0) {
        int success = call_posix_spawn(p, binary_path, &parse_result, error);
        command_parse_result_free(&parse_result);
        free(binary_path);
        return success ? PROCESS_CREATE_SUCCESS
                       : PROCESS_CREATE_ERROR_UNABLE_TO_SPAWN_PROCESS;
      }

      path = path_end + 1;
      free(binary_path);
    }

    *error = printf_to_string("command not found: %s", arg0);
    command_parse_result_free(&parse_result);
    return PROCESS_CREATE_ERROR_NOT_FOUND;
  } else {
    int relative = 0;
    const char *binary_path;
    if (arg0[0] == '/') {
      // absolute path
      relative = 0;
      binary_path = arg0;
    } else {
      relative = 1;
      const char *cwd = tinyshell_get_current_directory(shell);
      binary_path = printf_to_string("%s/%s", cwd, arg0);
    }
    // binary in current directory or absolute path to binary
    int return_value = PROCESS_CREATE_SUCCESS;
    if (access(binary_path, X_OK) == 0) {
      return_value = call_posix_spawn(p, binary_path, &parse_result, error)
                         ? PROCESS_CREATE_SUCCESS
                         : PROCESS_CREATE_ERROR_UNABLE_TO_SPAWN_PROCESS;
    } else {
      *error = printf_to_string("permission denied: %s", arg0);
      return_value = PROCESS_CREATE_ERROR_PERMISSION_DENIED;
    }

    command_parse_result_free(&parse_result);
    if (relative) {
      free((char *)binary_path);
    }
    return return_value;
  }

  return PROCESS_CREATE_SUCCESS;

fail_alloc_binary_path:
  command_parse_result_free(&parse_result);

  return PROCESS_CREATE_ERROR_OUT_OF_MEMORY;
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
