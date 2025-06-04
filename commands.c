#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <windows.h>
#include "commands.h"

void run_command(const char* command, char* outputBuffer, size_t outputBufferSize) {
    HANDLE hReadPipe, hWritePipe;
    SECURITY_ATTRIBUTES sa;
    PROCESS_INFORMATION pi;
    STARTUPINFOA si;
    BOOL bSuccess;
    DWORD dwRead;
    char readBuffer[256];

    if (outputBufferSize > 0) {
        outputBuffer[0] = '\0';
    }
    else {
        return;
    }

    sa.nLength = sizeof(SECURITY_ATTRIBUTES);
    sa.bInheritHandle = TRUE;
    sa.lpSecurityDescriptor = NULL;

    if (!CreatePipe(&hReadPipe, &hWritePipe, &sa, 0)) {
        snprintf(outputBuffer, outputBufferSize, "Failed to create pipe.\n");
        return;
    }

    if (!SetHandleInformation(hReadPipe, HANDLE_FLAG_INHERIT, 0)) {
        snprintf(outputBuffer, outputBufferSize, "Failed to set handle information for read pipe.\n");
        CloseHandle(hReadPipe);
        CloseHandle(hWritePipe);
        return;
    }

    ZeroMemory(&pi, sizeof(PROCESS_INFORMATION));
    ZeroMemory(&si, sizeof(STARTUPINFOA));
    si.cb = sizeof(STARTUPINFOA);
    si.hStdError = hWritePipe;
    si.hStdOutput = hWritePipe;
    si.dwFlags |= STARTF_USESTDHANDLES | STARTF_USESHOWWINDOW;
    si.wShowWindow = SW_HIDE;

    char fullCommandLine[1024];
    _snprintf_s(fullCommandLine, sizeof(fullCommandLine), _TRUNCATE, "cmd.exe /c %s", command);
    fullCommandLine[sizeof(fullCommandLine) - 1] = '\0';

    char* commandLine = _strdup(fullCommandLine);
    if (commandLine == NULL) {
        snprintf(outputBuffer, outputBufferSize, "Failed to duplicate command string.\n");
        CloseHandle(hReadPipe);
        CloseHandle(hWritePipe);
        return;
    }

    bSuccess = CreateProcessA(
        NULL,
        commandLine,
        NULL,
        NULL,
        TRUE,
        CREATE_NO_WINDOW,
        NULL,
        NULL,
        &si,
        &pi
    );

    free(commandLine);

    if (!bSuccess) {
        snprintf(outputBuffer, outputBufferSize, "Failed to run command (CreateProcess failed with error %lu).\n", GetLastError());
        CloseHandle(hReadPipe);
        CloseHandle(hWritePipe);
        return;
    }

    CloseHandle(hWritePipe);
    hWritePipe = NULL;

    size_t currentOutputLength = 0;
    while (TRUE) {
        bSuccess = ReadFile(hReadPipe, readBuffer, sizeof(readBuffer) - 1, &dwRead, NULL);
        if (!bSuccess || dwRead == 0) {
            break;
        }

        readBuffer[dwRead] = '\0';

        if (currentOutputLength + dwRead < outputBufferSize) {
            strcat_s(outputBuffer, outputBufferSize, readBuffer);
            currentOutputLength += dwRead;
        }
        else {
            size_t remainingSpace = outputBufferSize - currentOutputLength - 1;
            if (remainingSpace > 0) {
                strncat_s(outputBuffer, outputBufferSize, readBuffer, remainingSpace);
            }
            break;
        }
    }

    WaitForSingleObject(pi.hProcess, INFINITE);

    CloseHandle(pi.hProcess);
    CloseHandle(pi.hThread);
    CloseHandle(hReadPipe);
}
