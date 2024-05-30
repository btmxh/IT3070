#include <windows.h>
#include <stdio.h>
#include <string.h>

int main() {
    STARTUPINFO info;
    PROCESS_INFORMATION out;
    memset(&info, 0, sizeof(info));
    memset(&out, 0, sizeof(out));
    info.cb = sizeof(info);
    BOOL a = CreateProcess("Debug\\child.exe", NULL, NULL, NULL, FALSE, 0, NULL,NULL, &info,&out);

    if(a == FALSE) {
        printf("%lld", (long long)GetLastError());
        exit(1);
    }
    WaitForSingleObject(out.hProcess, 1000);
    TerminateProcess(out.hProcess, 1);
    CloseHandle(out.hProcess);
    CloseHandle(out.hThread);
        printf("%lld", (long long)GetLastError());

    return 0;
}
