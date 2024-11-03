#ifndef LOGGER_H
#define LOGGER_H

#include "mydefines.h"

//NOTE: Colors for VIRTUALTERMINAL ONLY WINDOWS (?)
#define YU_ALL_DEFAULT "\x1b[0m"
#define YU_BOLD "\x1b[1m"
#define YU_FG_BLACK "\x1b[30m"
#define YU_FG_RED "\x1b[31m"
#define YU_FG_GREEN "\x1b[32m"
#define YU_FG_YELLOW "\x1b[33m"
#define YU_FG_BLUE "\x1b[34m"
#define YU_FG_MAGENTA "\x1b[35m"
#define YU_FG_CYAN "\x1b[36m"
#define YU_FG_WHITE "\x1b[37m"
#define YU_FG_EXTENDED "\x1b[38m"
#define YU_FG_DEFAULT "\x1b[39m"
#define YU_BG_BLACK "\x1b[40m"
#define YU_BG_RED "\x1b[41m"
#define YU_BG_GREEN "\x1b[42m"
#define YU_BG_YELLOW "\x1b[43m"
#define YU_BG_BLUE "\x1b[44m"
#define YU_BG_MAGENTA "\x1b[45m"
#define YU_BG_CYAN "\x1b[46m"
#define YU_BG_WHITE "\x1b[47m"
#define YU_BG_EXTENDED "\x1b[48m"
#define YU_BG_DEFAULT "\x1b[49m"

#define SET_COLOR(i, r, g, b) "\x1b]4;" #i ";rgb:" #r "/" #g "/" #b "\x1b\\"

#define LS_FATAL YU_BOLD YU_BG_RED YU_FG_BLACK "[FATAL]: "
#define LS_ERROR YU_FG_RED "[ERROR]: "
#define LS_WARN YU_FG_YELLOW "[WARN]: "
#define LS_INFO YU_FG_BLUE "[INFO]: "
#define LS_DEBUG YU_FG_GREEN "[DEBUG]: "
#define LS_TRACE YU_FG_MAGENTA "[TRACE]: "

// NOTE: Disable debug and trace logging for release builds.
#if YURELEASE == 1
	#define LOG_DEBUG_ENABLED 0
	#define LOG_TRACE_ENABLED 0
#else
	#define LOG_DEBUG_ENABLED 1
	#define LOG_TRACE_ENABLED 1
#endif // YURELEASE

#define LOG_WARN_ENABLED 1
#define LOG_INFO_ENABLED 1

typedef enum LogLevel
{
     LOG_LEVEL_FATAL	= 0,
	 LOG_LEVEL_ERROR	= 1,
	 LOG_LEVEL_WARN		= 2,
	 LOG_LEVEL_INFO		= 3,
	 LOG_LEVEL_DEBUG	= 4,
	 LOG_LEVEL_TRACE	= 5,
	 MAX_LOG_LEVEL
} LogLevel;

b8		LoggingInit(void);
void	LoggingShutdown(void);

void LogOutputLineAndFile(
		LogLevel							level,
		char*								pFilePath,
		int									line,
		const char*							message,
		...									);

void LogOutput(
		LogLevel							level,
		const char*							message,
		...									);

#if LOG_INFO_ENABLED == 1
	#define YINFO(message, ...) LogOutput(LOG_LEVEL_INFO, message, ##__VA_ARGS__);
#else
	#define YINFO(message, ...)
#endif

#if YU_RELEASE == 1
	#define YFATAL(message, ...) LogOutput(LOG_LEVEL_FATAL, message, ##__VA_ARGS__);
	#define YERROR(message, ...) LogOutput(LOG_LEVEL_ERROR, message, ##__VA_ARGS__);

	#if LOG_WARN_ENABLED == 1
		#define YWARN(message, ...) LogOutput(LOG_LEVEL_WARN, message, ##__VA_ARGS__);
	#else
		#define YWARN(message, ...)
	#endif
#else
	#define YFATAL(message, ...) LogOutputLineAndFile(LOG_LEVEL_FATAL, __FILE__, __LINE__, message, ##__VA_ARGS__);
	#define YERROR(message, ...) LogOutputLineAndFile(LOG_LEVEL_ERROR, __FILE__, __LINE__, message, ##__VA_ARGS__);

	#if LOG_WARN_ENABLED == 1
		#define YWARN(message, ...) LogOutputLineAndFile(LOG_LEVEL_WARN, __FILE__, __LINE__, message, ##__VA_ARGS__);
	#else
		#define YWARN(message, ...)
	#endif
#endif //YURELEASE

#if LOG_DEBUG_ENABLED == 1
	#define YDEBUG(message, ...) LogOutputLineAndFile(LOG_LEVEL_DEBUG, __FILE__, __LINE__, message, ##__VA_ARGS__);
#else
	#define YDEBUG(message, ...)
#endif

#if LOG_TRACE_ENABLED == 1
	#define YTRACE(message, ...) LogOutputLineAndFile(LOG_LEVEL_TRACE, __FILE__, __LINE__, message, ##__VA_ARGS__);
#else
	#define YTRACE(message, ...)
#endif

#endif // LOGGER_H
