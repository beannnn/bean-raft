
#include <stdio.h>
#include <atomic>

#define LOG_DEBUG   0
#define LOG_INFO    1
#define LOG_WARNING 2
#define LOG_ERROR   3
#define LOG_FATAL   4

void logfunction(int loglevel, const char *file, int line, const char *fmt, ...);
long long logUstime(void);

#define LOG_CONCAT(a, b) a##b

#define logWithLevel(logleve, fmt, args...)  \
    logfunction(loglevel, __FILE__, __LINE__, fmt, ##args)

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

/* Carefully call this log function. */
#define logEverySeconds(level, seconds, ...) do {  \
    static std::atomic<long long> LOG_CONCAT(log_static_, __LINE__)(0);  \
    long long LOG_CONCAT(log_current, __LINE__) = logUstime();  \
    long long LOG_CONCAT(log_seen, __LINE__) = std::atomic_load_explicit(&LOG_CONCAT(log_static_, __LINE__), std::memory_order_relaxed); \
    if (LOG_CONCAT(log_current, __LINE__) >= (LOG_CONCAT(log_seen, __LINE__) + seconds) &&  \
        LOG_CONCAT(log_static_, __LINE__).compare_exchange_weak(  \
            LOG_CONCAT(log_seen, __LINE__),  \
            LOG_CONCAT(log_current, __LINE__))) {  \
            logWithLevel(level, __VA_ARGS__);  \
        }  \
} while(0)

void initLoggingSystem(const char *filename);
void deinitLoggingSystem();
void setThreadLocalIdentifier(char identifier);
void setLogLevel(int loglevel);
void setLogSyncLevel(int loglevel);
void setMaxMemorySize(size_t kb);
void setMaxFileSize(size_t mb);
void setLogColor(bool set_color);
