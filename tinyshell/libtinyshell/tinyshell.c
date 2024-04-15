#include "tinyshell.h"
#include "parse_cmd.h"
#include <builtin.h>
#include <errno.h>
#include <process.h>
#include <stdio.h>
#include <stdlib.h>
#include <utils.h>

#ifdef _WIN32
#include <direct.h>
#else
#include <unistd.h>
#endif

static char *get_command(int *exit) {
  void *command = NULL;
  int len = 0;
  int cap = 0;
  int ch;

  do {
    ch = fgetc(stdin);
    if (ch == '\n') {
      ch = '\0';
    }
    if (ch == EOF) {
      ch = '\0';
      *exit = 1;
    }

    char c = (char)ch;
    if (!vecpush(&command, &len, &cap, 1, &c, 1)) {
      goto fail_vecpush;
    }
  } while (ch != '\0');

  return command;

fail_vecpush:
  free(command);

  return NULL;
}

char *get_current_directory() {
  int size = 128;
  char *buffer = NULL;
  while (1) {
    char *r_buffer = realloc(buffer, size);
    if (!r_buffer) {
      break;
    }
    buffer = r_buffer;

    if (!getcwd(r_buffer, size)) {
      if (errno == ERANGE) {
        size *= 2;
        continue;
      }

      break;
    }

    return buffer;
  }

  free(buffer);
  return NULL;
}

// Ham nay de tao ra tinyshell moi
int tinyshell_new(tinyshell *shell) { return 1; }

static void process_command(tinyshell *shell, const char *command) {
  command_parse_result parse_result;
  char *error_msg;
  if (!parse_command(command, &parse_result, &error_msg)) {
    if (!error_msg) {
      printf("invalid command\n");
    } else {
      printf("invalid command: %s\n", error_msg);
    }

    return;
  }

  if (parse_result.argc == 0) {
    goto fail;
  }

  int status_code = 0;
  if (try_run_builtin(shell, &parse_result, &status_code)) {
    goto check_status_code;
  }

  char *binary_path = find_executable(parse_result.argv[0], shell);
  if (!binary_path) {
    printf("executable not found: %s\n", parse_result.argv[0]);
    goto fail;
  }

  process p;
  int foreground;
  if (!process_create(&p, binary_path, shell, command, &parse_result,
                      &error_msg)) {
    if (error_msg != NULL) {
      printf("%s\n", error_msg);
    } else {
      printf("unable to spawn process\n");
    }

    free(binary_path);
    goto fail;
  }

  if (parse_result.foreground) {
    process_wait_for(&p, &status_code);
    process_free(&p);
  }

check_status_code:
  if (status_code != 0) {
    printf("process exited with error code %d\n", status_code);
  }

  return;
fail:
  command_parse_result_free(&parse_result);
}

// Ham nay de chay tinyshell
int tinyshell_run(tinyshell *shell) {
  printf("Hello, World!\n");

  int exit = 0;
  while (!exit) {
    printf("$ ");
    char *command = get_command(&exit);
    process_command(shell, command);
    free(command);
  }

  return 1;
}

void tinyshell_destroy(tinyshell *shell) {}

const char *tinyshell_get_path_env(const tinyshell *shell) { return ""; }
