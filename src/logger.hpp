
#include <stdio.h>

#define LOG_DEBUG   0
#define LOG_INFO    1
#define LOG_WARNING 2
#define LOG_ERROR   3
#define LOG_FATAL   4
#define LOG_RAW     (1 << 5)

void logfunction(int loglevel, const char *file, int line, const char *fmt, ...);

#define logDebug(fmt, args...)    \
    logfunction(LOG_DEBUG, __FILE__, __LINE__, fmt, ##args)

#define logInfo(fmt, args...)    \
    logfunction(LOG_INFO, __FILE__, __LINE__, fmt, ##args)

#define logWarning(fmt, args...)    \
    logfunction(LOG_WARNING, __FILE__, __LINE__, fmt, ##args)

#define logError(fmt, args...)    \
    logfunction(LOG_ERROR, __FILE__, __LINE__, fmt, ##args)

#define logFatal(fmt, args...)    \
    logfunction(LOG_FATAL, __FILE__, __LINE__, fmt, ##args)

// #define logEverySeconds(loglevel, fmt, args...) \

// need do something for logfatal.

#define logRaw(fmt, args...) \
    logfunction(LOG_RAW, NULL, 0, fmt, ##args);

// 用namespace封装就可以了.
void initLoggingSystem(const char *filename);
void deinitLoggingSystem();
// void setHeader();
void setMaxMemtableSize(size_t kb);
void setMaxFileSize(size_t mb);
void setSyncLogLevel();
void setLogLevel();
void set
