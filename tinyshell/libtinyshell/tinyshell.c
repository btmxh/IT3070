#include "tinyshell.h"
#include <stdio.h>
#include <process.h>
#include <stdlib.h>
#include <utils.h>

// Ham nay de tao ra tinyshell moi
int tinyshell_new(tinyshell *shell)
{
  return 1;
}

// Ham nay de chay tinyshell
int tinyshell_run(tinyshell *shell)
{
  printf("Hello, World!\n");

  int exit = 0;
  while (!exit)
  {
    printf("$ ");

    char* command = NULL;
    int length = 0;
    int capacity = 0;

    while(1) {
      int ch = fgetc(stdin);
      if(ch == '\n') {
        ch = '\0';
      }

      if(ch == EOF) {
        ch = '\0';
        exit = 1;
      }

      char c = (char) ch;

      if(vecpush((void**)&command, &length, &capacity, sizeof(char), &c, 1) == 0) {
        free(command);
        return 0;
      }

      if(c == '\0') {
        break;
      }
    }

    process p;
    char *error_msg;
    process_create_error error = process_create(&p, shell, command, &error_msg);
    free(command);

    if (error == PROCESS_CREATE_ERROR_EMPTY_COMMAND)
    {
      continue;
    }
    else if (error == PROCESS_CREATE_ERROR_INVALID_COMMAND)
    {
      if (error_msg != NULL)
      {
        printf("invalid command: %s\n", error_msg);
      }
      else
      {
        printf("invalid command\n");
      }

      free(error_msg);
      continue;
    }
    else if (error == PROCESS_CREATE_ERROR_OUT_OF_MEMORY)
    {
      if (error_msg != NULL)
      {
        printf("out of memory: %s\n", error_msg);
      }
      else
      {
        printf("out of memory\n");
      }

      free(error_msg);
      continue;
    }
    else if (error == PROCESS_CREATE_ERROR_UNABLE_TO_SPAWN_PROCESS)
    {
      if (error_msg != NULL)
      {
        printf("%s\n", error_msg);
      }
      else
      {
        printf("unable to spawn process\n");
      }

      free(error_msg);
      continue;
    }

    int status_code;
    process_wait_for(&p, &status_code);
  }

  return 1;
}

//
void tinyshell_destroy(tinyshell *shell)
{
}

const char *tinyshell_get_current_directory(const tinyshell *shell)
{
  return ".";
}

const char *tinyshell_get_path_env(const tinyshell *shell)
{
  return "";
}
