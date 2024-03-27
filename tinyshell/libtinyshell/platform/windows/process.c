#include "../../process.h"
#include <utils.h>

int process_create(process *p, const tinyshell *shell, const char *command,
                   char **error)
{
    STARTUPINFO info;
    memset(&info, 0, sizeof(info));
    info.cb = sizeof(info);
    char *command_copy = printf_to_string("%s", command);
    if (CreateProcess(NULL, command_copy, NULL, NULL, FALSE, 0, NULL,
                      tinyshell_get_current_directory(shell), &info, p))
    {
        return 1;
    }
    else
    {
        free(command_copy);
        return 0;
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