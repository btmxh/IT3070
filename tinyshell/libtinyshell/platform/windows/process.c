#include "../../process.h"
#include "tinyshell.h"
#include <parse_cmd.h>
#include <string.h>
#include <utils.h>

static int file_exists(const char *path) {
  DWORD a = GetFileAttributesA(path);
  if (a == INVALID_FILE_ATTRIBUTES) {
    return 0;
  } else {
    return (a & FILE_ATTRIBUTE_DIRECTORY) == 0;
  }
};

char *search_directory_for_executable(const char *arg0, const char *directory) {
  char *extensions[] = {"", ".cmd", ".exe", ".bat", ".com"};
  for (int i = 0; i < sizeof(extensions) / sizeof(extensions[0]); i++) {
    char *file_path;
    if (directory) {
      file_path = printf_to_string("%s\\%s%s", directory, arg0, extensions[i]);
    } else {
      file_path = printf_to_string("%s%s", arg0, extensions[i]);
    }

    if (file_exists(file_path)) {
      return file_path;
    }

    free(file_path);
  }

  return NULL;
}

char *find_executable(const char *arg0, const tinyshell *shell) {
  char *executable = search_directory_for_executable(arg0, NULL);
  if (executable) {
    return executable;
  }

  if (strchr(arg0, '/') != NULL || strchr(arg0, '\\') != NULL) {
    return NULL;
  }

  char *path = printf_to_string("%s", tinyshell_get_path_env(shell));
  if (!path) {
    return NULL;
  }

  char *saveptr;
  for (char *dir = reentrant_strtok(path, ";", &saveptr); dir;
       dir = reentrant_strtok(NULL, ";", &saveptr)) {
    executable = search_directory_for_executable(arg0, dir);
    if (executable) {
      return executable;
    }
  }

  return NULL;
}

int process_create(process *p, char *binary_path, const tinyshell *shell,
                   const char *command, command_parse_result *parse_result,
                   char **error) {
  char *command_copy = printf_to_string("%s", command);
  if (!command_copy) {
    return 0;
  }

  if (!parse_result->foreground) {
    char *ampersand = strrchr(command_copy, '&');
    if (ampersand != NULL) {
      *ampersand = ' ';
    }
  }

  // for batch files, we follow the advice in the Win32 documentation page of
  // CreateProcess
  if (string_ends_with(binary_path, ".bat")) {
    char *bat_command = printf_to_string("cmd.exe /c %s", command_copy);
    char *new_binary_path = printf_to_string("cmd.exe");

    if (!bat_command || !new_binary_path) {
      free(command_copy);
      free(binary_path);
      free(bat_command);
      free(new_binary_path);
      *error = printf_to_string("out of memory");
      return 1;
    }

    free(command_copy);
    command_copy = bat_command;
    free(binary_path);
    binary_path = new_binary_path;
  }

  STARTUPINFO info;
  memset(&info, 0, sizeof(info));
  info.cb = sizeof(info);
  int x;
  printf("%p\n", command_copy);
  printf("%p\n", &x);
  printf("%p\n", binary_path);
  if (CreateProcess(binary_path, command_copy, NULL, NULL, FALSE, 0, NULL, NULL,
                    &info, p)) {
    command_parse_result_free(parse_result);
    return 1;
  }

  free(command_copy);
  return 0;
}

void process_free(process *p) {
  CloseHandle(p->hProcess);
  CloseHandle(p->hThread);
}

// blocking
int process_wait_for(process *p, int *status_code) {
  DWORD result = WaitForSingleObject(p->hProcess, INFINITE);
  if (result == WAIT_OBJECT_0) {
    if (status_code) {
      *status_code = 0;
    }
    return 1;
  } else {
    return 0;
  }
}

// non-blocking
int process_try_wait_for(process *p, int *status_code, int *done) {
  DWORD result = WaitForSingleObject(p->hProcess, 0);
  if (result == WAIT_TIMEOUT) {
    *done = 0;
    return 1;
  } else if (result == WAIT_OBJECT_0) {
    *done = 1;
    *status_code = 0;
    return 1;
  } else {
    return 0;
  }
}

int process_kill(process *p) { return TerminateProcess(p->hProcess, 0); }
int process_suspend(process *p) { return DebugActiveProcess(p->dwProcessId); }
int process_resume(process *p) {
  return DebugActiveProcessStop(p->dwProcessId);
}
