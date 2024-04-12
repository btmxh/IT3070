#include "../../process.h"
#include <utils.h>
#include <parse_cmd.h>

static int file_exists(const char *path)
{
    DWORD a = GetFileAttributesA(path);
    if (a == INVALID_FILE_ATTRIBUTES)
    {
        return 0;
    }
    else
    {
        return (a & FILE_ATTRIBUTE_DIRECTORY) == 0;
    }
};

static char *find_executable(tinyshell *shell, const char *program)
{
    const char *path = tinyshell_get_path_env(shell);
    char *path_mut = printf_to_string("%s", path);
    char *token = strtok(path_mut, ";");

    while (token != NULL)
    {
        char *extensions[] = {"cmd", "exe", "bat", "com"};
        for (int i = 0; i < 4; i++)
        {
            char *file_path = printf_to_string("%s\\%s.%s", token, program, extensions[i]);

            if (file_exists(file_path))
            {
                free(path_mut);
                return file_path;
            }

            free(file_path);
        }

        token = strtok(NULL, ";");
    }

    free(path_mut);
    return NULL;
}

process_create_error process_create(process *p, const tinyshell *shell,
                                    const char *command, char **error,
                                    int* foreground)
{
    *error = NULL;
    STARTUPINFO info;
    memset(&info, 0, sizeof(info));
    info.cb = sizeof(info);

    command_parse_result result;
    if(!parse_command(command, &result, error)) {
        return PROCESS_CREATE_ERROR_INVALID_COMMAND;
    }

    *foreground = result.foreground;

    if(result.argc == 0) {
        command_parse_result_free(&result);
        return PROCESS_CREATE_ERROR_EMPTY_COMMAND;
    }

    char *app = find_executable(shell, result.argv[0]);
    char *command_copy = printf_to_string("%s", command);

    if(result.foreground) {
        char* ampersand = strrchr(command_copy, '&');
        if(ampersand != NULL) {
            *ampersand = ' ';
        }
    }

    command_parse_result_free(&result);
    if (CreateProcess(app, command_copy, NULL, NULL, FALSE, 0, NULL,
                      tinyshell_get_current_directory(shell), &info, p))
    {
        return PROCESS_CREATE_SUCCESS;
    }
    else
    {
        free(app);
        free(command_copy);
        return PROCESS_CREATE_ERROR_UNABLE_TO_SPAWN_PROCESS;
    }
}
void process_free(process *p)
{
    CloseHandle(p->hProcess);
    CloseHandle(p->hThread);
}

// blocking
int process_wait_for(process *p, int *status_code)
{
    DWORD result = WaitForSingleObject(p->hProcess, INFINITE);
    if (result == WAIT_OBJECT_0)
    {
        *status_code = 0;
        return 1;
    }
    else
    {
        return 0;
    }
}

// non-blocking
int process_try_wait_for(process *p, int *status_code, int *done)
{
    DWORD result = WaitForSingleObject(p->hProcess, 0);
    if (result == WAIT_TIMEOUT)
    {
        *done = 0;
        return 1;
    }
    else if (result == WAIT_OBJECT_0)
    {
        *done = 1;
        *status_code = 0;
        return 1;
    }
    else
    {
        return 0;
    }
}

int process_kill(process *p)
{
    return TerminateProcess(p->hProcess, 0);
}
int process_suspend(process *p)
{
    return DebugActiveProcess(p->dwProcessId);
}
int process_resume(process *p)
{
    return DebugActiveProcessStop(p->dwProcessId);
}