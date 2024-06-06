#include "tinyshell.h"
#include "parse_cmd.h"
#include <builtin.h>
#include <errno.h>
#include <process.h>
#include <signal.h>
#include <stdio.h>
#include <stdlib.h>
#include <utils.h>

#ifdef WIN32
#include <direct.h>
#include <io.h>
#else
#include <unistd.h>
#endif

static char *get_command(tinyshell *shell) {
  char *command = NULL;
  int len = 0;
  int cap = 0;
  int ch;

  do {
    ch = fgetc(shell->input);
    if (ch == '\n') {
      ch = '\0';
    }
    if (ch == EOF) {
      ch = '\0';
      shell->exit = 1;
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

    if (!POSIX_WIN32(getcwd)(r_buffer, size)) {
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

static tinyshell *current_shell;

static void sigint_handler(int s) {
  signal(SIGINT, sigint_handler);
  if (current_shell->has_fg) {
    process_kill(&current_shell->fg);
  }
}

// mutex lock/unlock failure basically never happen
void tinyshell_lock_bg_procs(tinyshell *shell) {
  int ret = mtx_lock(&shell->bg_lock);
  if (ret != thrd_success) {
    exit(1);
  }
}

void tinyshell_unlock_bg_procs(tinyshell *shell) {
  int ret = mtx_unlock(&shell->bg_lock);
  if (ret != thrd_success) {
    exit(1);
  }
}

static void update_jobs(tinyshell *shell) {
  tinyshell_lock_bg_procs(shell);
  for (int i = 0; i < shell->bg_cap; ++i) {
    if (shell->bg[i].status == BG_PROCESS_FINISHED) {
      // join the background process thread
      thrd_join(shell->bg[i].thread, NULL);
      shell->bg[i].status = BG_PROCESS_EMPTY;
    }
  }
  tinyshell_unlock_bg_procs(shell);
}

static int find_bg_job_index(tinyshell *shell, int *index) {
  update_jobs(shell);
  tinyshell_lock_bg_procs(shell);
  for (int i = 0; i < shell->bg_cap; ++i) {
    if (shell->bg[i].status == BG_PROCESS_EMPTY) {
      if (index) {
        *index = i;
      }

      tinyshell_unlock_bg_procs(shell);
      return 1;
    }
  }

  // reallocation needed
  int new_cap = shell->bg_cap * 2 + 1;
  bg_process *new_bg = realloc(shell->bg, new_cap * sizeof *new_bg);
  if (!new_bg) {
    tinyshell_unlock_bg_procs(shell);
    return 0;
  }

  if (index) {
    *index = shell->bg_cap;
  }

  for (int i = shell->bg_cap; i < new_cap; ++i) {
    // mark empty slots as finished processes
    new_bg[i].status = BG_PROCESS_EMPTY;
  }

  shell->bg_cap = new_cap;
  shell->bg = new_bg;
  tinyshell_unlock_bg_procs(shell);

  return 1;
}

static int bg_process_thread(void *data) {
  int index = (int)(size_t)data;

  tinyshell_lock_bg_procs(current_shell);
  process p = current_shell->bg[index].p;
  tinyshell_unlock_bg_procs(current_shell);

  int status_code;
  if (!process_wait_for(&p, &status_code)) {
    status_code = -1;
  }

  printf("job %%%d exited with error code %d\n", index + 1, status_code);

  tinyshell_lock_bg_procs(current_shell);
  bg_process *bg = &current_shell->bg[index];
  process_free(&bg->p);
  free(bg->cmd);
  bg->status = BG_PROCESS_FINISHED;
  tinyshell_unlock_bg_procs(current_shell);

  return status_code;
}

// Ham nay de tao ra tinyshell moi
int tinyshell_new(tinyshell *shell, FILE *input) {
  current_shell = shell;
  signal(SIGINT, sigint_handler);
  shell->has_fg = 0;
  shell->exit = false;
  shell->bg = NULL;
  if (mtx_init(&shell->bg_lock, mtx_plain) != thrd_success) {
    printf("unable to initialize jobs lock\n");
    return 0;
  }
  shell->bg_cap = 0;
  shell->path = NULL;
  shell->input = input;
  return 1;
}

static void process_command(tinyshell *shell, const char *command,
                            int *status_code_ret);

static char *read_file(const char *path) {
  FILE *f = NULL;
  char *buffer = NULL;

  f = fopen(path, "rb");
  if (!f) {
    printf("unable to open script file: %s\n", path);
    return NULL;
  }

  fseek(f, 0, SEEK_END);
  int filesize = ftell(f);
  fseek(f, 0, SEEK_SET);

  if (ferror(f) || filesize < 0) {
    printf("unable to determine filesize of script file: %s\n", path);
    goto fail;
  }

  buffer = malloc(filesize + 1);
  if (!buffer) {
    printf("unable to allocate buffer for script file: %s\n", path);
    goto fail;
  }

  if (fread(buffer, 1, filesize, f) != filesize || ferror(f)) {
    printf("unable to read script file: %s\n", path);
    goto fail;
  }

  fclose(f);

  buffer[filesize] = '\0';
  return buffer;

fail:
  free(buffer);
  if (f) {
    fclose(f);
  }
  return NULL;
}

static int try_run_script(tinyshell *shell, const char *path,
                          int *status_code) {
#ifdef WIN32
  const char extension[] = ".tbat";
#else
  if (!strchr(path, '/')) {
    return 0;
  }
  const char extension[] = ".tsh";
#endif
  int ext_len = (int)sizeof(extension) - 1, path_len = (int)strlen(path);
  if (path_len < ext_len || strcmp(&path[path_len - ext_len], extension) != 0) {
    return 0;
  }

  char *script_content = read_file(path);
  if (!script_content) {
    return 0;
  }

  char *saveptr;
  for (char *cmd = reentrant_strtok(script_content, "\n", &saveptr); cmd;
       cmd = reentrant_strtok(NULL, "\n", &saveptr)) {
    process_command(shell, cmd, status_code);
  }

  free(script_content);
  return 1;
}

static void process_command(tinyshell *shell, const char *command,
                            int *status_code_ret) {
  command_parse_result parse_result;
  char *error_msg = NULL;
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
  const char *type = "builtin command";
  if (try_run_builtin(shell, &parse_result, &status_code)) {
    goto check_status_code;
  }

  type = "script";
  if (try_run_script(shell, parse_result.argv[0], &status_code)) {
    command_parse_result_free(&parse_result);
    goto check_status_code;
  }

  type = "process";
  char *binary_path = find_executable(parse_result.argv[0], shell);
  if (!binary_path) {
    printf("executable not found: %s\n", parse_result.argv[0]);
    goto fail;
  }

  int bg_job_index = -1;
  if (!parse_result.foreground) {
    if (!find_bg_job_index(shell, &bg_job_index)) {
      printf("unable to determine job index for background process");
      goto fail;
    }
  }

  process p;
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
    shell->has_fg = 1;
    shell->fg = p;
    process_wait_for(&p, &status_code);
    shell->has_fg = 0;
    process_free(&p);
  } else {
    tinyshell_lock_bg_procs(shell);
    bg_process *bg = &shell->bg[bg_job_index];
    bg->p = p;
    bg->status = BG_PROCESS_RUNNING;
    bg->cmd = printf_to_string("%s", command);
    // FIXME: properly allocate bg_job_index
    thrd_create(&bg->thread, bg_process_thread, (void *)(size_t)bg_job_index);
    tinyshell_unlock_bg_procs(shell);
  }

check_status_code:
  if (status_code_ret) {
    *status_code_ret = status_code;
  } else {
    if (status_code != 0) {
      printf("%s exited with error code %d\n", type, status_code);
    }
  }

  return;
fail:
  command_parse_result_free(&parse_result);
}

// Ham nay de chay tinyshell
int tinyshell_run(tinyshell *shell) {
  while (!shell->exit) {
    update_jobs(shell);
#ifdef WIN32
    char *cwd = get_current_directory();
    printf("%s>", cwd);
    free(cwd);
#else
    printf("$ ");
#endif
    char *command = get_command(shell);
    if (!POSIX_WIN32(isatty)(POSIX_WIN32(fileno)(shell->input))) {
      puts(command);
    }
    process_command(shell, command, NULL);
    free(command);
    puts("");
  }

  return 1;
}

void tinyshell_destroy(tinyshell *shell) {
  tinyshell_lock_bg_procs(shell);
  for (int i = 0; i < shell->bg_cap; ++i) {
    if (shell->bg[i].status == BG_PROCESS_EMPTY) {
      continue;
    }

    if (shell->bg[i].status != BG_PROCESS_FINISHED) {
      process_kill(&shell->bg[i].p);
    }

    tinyshell_unlock_bg_procs(shell);
    thrd_join(shell->bg[i].thread, NULL);
    tinyshell_lock_bg_procs(shell);
  }
  tinyshell_unlock_bg_procs(shell);
  mtx_destroy(&shell->bg_lock);

  free(shell->bg);
  free(shell->path);
}

const char *tinyshell_get_path_env(const tinyshell *shell) {
  return shell->path ? shell->path : "";
}
