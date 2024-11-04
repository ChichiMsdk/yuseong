#include "core/logger.h"
#include "core/assert.h"
#include "os.h"

#if PLATFORM_WINDOWS
#	define WIN32_LEAN_AND_MEAN
#	include <windows.h>
#	define FDINPUT STD_INPUT_HANDLE
#	define FDOUTPUT STD_OUTPUT_HANDLE
#	define FDERROR STD_ERROR_HANDLE
#elif PLATFORM_LINUX
#	include <unistd.h>
#	define FDINPUT STDIN_FILENO
#	define FDOUTPUT STDOUT_FILENO
#	define FDERROR STDERR_FILENO

#	define STDINPUT stdin
#	define STDOUTPUT stdout
#	define STDERROR stderr
#endif

// TODO: temporary

#include <stdio.h>
#include <string.h>
#include <stdarg.h>

void
ReportAssertionFailure(const char *pExpression, const char *pMessage, const char *pFile, int32_t line)
{
	if (strlen(pMessage) == 0)
	{
		LogOutput(LOG_LEVEL_FATAL, "Assertion Failure: %s, in file: %s, line: %d",
			pExpression, pFile, line);
	}
	else
	{
		LogOutput(LOG_LEVEL_FATAL, "Assertion Failure: %s, message: '%s', in file: %s, line: %d",
			pExpression, pMessage, pFile, line);
	}
}

b8
LoggingInit(void)
{
	return TRUE;
}

void
LoggingShutdown(void)
{
}

#define MSG_LENGTH 3200

/**
 * @brief			Prints `pMessage` in the `level` corresponding output 
 *
 * @param level		`LS_FATAL, LS_ERROR, LS_WARN, LS_INFO, LS_DEBUG, LS_TRACE`
 * @param pMessage	Truncated if length exceeds 3200 
 */
void
LogOutput(LogLevel level, const char *pMessage, ...)
{
	const char *pLevelStrings[] = {LS_FATAL, LS_ERROR, LS_WARN, LS_INFO , LS_DEBUG , LS_TRACE, LS_LEAKS, LS_NONE};
	b8 bError = level < LOG_LEVEL_WARN;

	/* NOTE: care here */
	char pOutMessage[MSG_LENGTH];

	/* NOTE: microsoft bullshit so if using clang __builtin_va_list */
	va_list argPtr;
	va_start(argPtr, pMessage);
	vsnprintf(pOutMessage, MSG_LENGTH, pMessage, argPtr);
	va_end(argPtr);

	char pOutMessage2[MSG_LENGTH];
	if (level == LOG_LEVEL_FATAL)
	{
		int count = sprintf(pOutMessage2, "%s%s", pLevelStrings[level], pOutMessage);
		sprintf(pOutMessage2 + count, "%s\n", YU_ALL_DEFAULT);
	}
	else
		sprintf(pOutMessage2, "%s%s%s\n", pLevelStrings[level], pOutMessage, YU_ALL_DEFAULT);
	if (bError)
		OsWrite(pOutMessage2, FDERROR);
	else
		OsWrite(pOutMessage2, FDOUTPUT);
}
/**
 * @brief			Prints `pMessage` in the `level` corresponding output with
 *					file and line added
 *
 * @param level		`LS_FATAL, LS_ERROR, LS_WARN, LS_INFO, LS_DEBUG, LS_TRACE`
 * @param pMessage	Truncated if length exceeds 3200 
 */
void
LogOutputLineAndFile(LogLevel level, char *pFilePath, int line, const char *pMessage, ...)
{
	const char *pLevelStrings[] = {LS_FATAL, LS_ERROR, LS_WARN, LS_INFO , LS_DEBUG , LS_TRACE, LS_LEAKS};
	b8 bError = level < LOG_LEVEL_WARN;

	/* NOTE: care here */
	char pOutMessage[MSG_LENGTH];

	/* NOTE: microsoft bullshit so if using clang __builtin_va_list */
	va_list argPtr;
	va_start(argPtr, pMessage);
	vsnprintf(pOutMessage, MSG_LENGTH, pMessage, argPtr);
	va_end(argPtr);

	char pOutMessage2[MSG_LENGTH];
	if (level == LOG_LEVEL_FATAL)
	{
		int count = sprintf(pOutMessage2, "%s\"%s\" in %s:%d", pLevelStrings[level], pOutMessage, pFilePath, line);
		sprintf(pOutMessage2 + count, "%s\n", YU_ALL_DEFAULT);
	}
	else
		sprintf(pOutMessage2, "%s\"%s\" in %s:%d %s\n", pLevelStrings[level], pOutMessage, pFilePath, line, YU_ALL_DEFAULT);
	if (bError)
		OsWrite(pOutMessage2, FDERROR);
	else
		OsWrite(pOutMessage2, FDOUTPUT);
}
