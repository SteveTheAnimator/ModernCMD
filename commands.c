#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "commands.h"

void run_command(const char* command, char* outputBuffer, size_t outputBufferSize) {
    FILE* pipe = _popen(command, "r");
    if (!pipe) {
        snprintf(outputBuffer, outputBufferSize, "Failed to run command.\n");
        return;
    }

    char line[256];
    outputBuffer[0] = '\0';

    while (fgets(line, sizeof(line), pipe)) {
        strncat_s(outputBuffer, outputBufferSize, line, _TRUNCATE);
    }

    _pclose(pipe);
}
