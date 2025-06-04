#ifndef COMMANDS_H
#define COMMANDS_H

#ifdef __cplusplus
extern "C" {
#endif

	void run_command(const char* command, char* outputBuffer, size_t outputBufferSize);

#ifdef __cplusplus
}
#endif

#endif
