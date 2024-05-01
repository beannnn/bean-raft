
#include "logger.hpp"

#include <stdarg.h>
#include <unistd.h>
#include <string.h>
#include <fcntl.h>
#include <time.h>
#include <sys/time.h>

#include <thread>
#include <condition_variable>
#include <mutex>
#include <string>
#include <list>
#include <iostream>
#include <sstream>
#include <iomanip>
#include <utility>

using namespace std;

#define LOG_COLOR_RED       31
#define LOG_COLOR_GREEN     32
#define LOG_COLOR_YELLOW    33
#define LOG_COLOR_BLUE      34
#define LOG_COLOR_MAGENTA   35
#define LOG_COLOR_CYAN      36
#define LOG_COLOR_GRAY      37
#define LOG_COLOR_DEFAULT   39 /* Default foreground color */

static const char *loglevels[5] = {
    "DEBUG",
    "INFO ",
    "WARN ",
    "ERROR",
    "FATAL"
};

static const int logcolors[5] = {
    LOG_COLOR_GRAY,
    LOG_COLOR_GREEN,
    LOG_COLOR_YELLOW,
    LOG_COLOR_RED,
    LOG_COLOR_RED
};

static constexpr int MAX_LOG_BUFFER_SIZE = 4096;        /* max size for normal log */
static constexpr int MAX_LOG_ENTRY = 2048;              /* max memory buffer */
static constexpr int PAGE_SIZE = 4096;                  /* linux file pagecache size */
static constexpr uint64_t DEFAULT_MAX_MEMORY_SIZE = (1ull << 20);  /* default 1MB */
static constexpr uint64_t DEFAULT_MAX_FILE_SIZE = 4 * (1ull << 30);  /* default 4GB */

thread_local char thread_identifier = ' ';  /* default nothing. */

long long logUstime(void) {
    struct timeval tv;
    gettimeofday(&tv, NULL);
    long long ust = static_cast<long long>(tv.tv_sec) * 1000000;
    ust += tv.tv_usec;
    return ust;
}

static void formatUnixTime(struct tm *tm_info) {
    struct timeval tv;
    gettimeofday(&tv, NULL);
    time_t timestamp = static_cast<time_t>(tv.tv_sec + (int)((double)tv.tv_usec * 0.000001));
    localtime_r(&timestamp, tm_info);
}

static const char* constBasename(const char* filepath) {
  const char* base = strrchr(filepath, '/');
  return base ? (base + 1) : filepath;
}

static std::string hostname() {
    char name[1024];
    if (gethostname(name, 1024) == 0) {
        return string(name);
    }
    return string();
}

class LoggerAllocator {
public:
    LoggerAllocator() {
        for (int i = 0; i < MAX_LOG_ENTRY; ++i) {
            void *p = malloc(MAX_LOG_BUFFER_SIZE);
            freelist.push_back(p);
        }
    }
    ~LoggerAllocator() {
        for (auto p : freelist) {
            free(p);
        }
    }
    void *allocate() {
        lock_guard<mutex> lk(mu);
        if (freelist.empty()) {
            return nullptr;
        }

        auto ret = freelist.front();
        freelist.pop_front();
        return ret;
    }
    void deallocate(void *p) {
        freelist.push_back(p);
    }
private:
    mutex mu;
    list<void *> freelist;
};

class FileWritter {
public:
    FileWritter(const char *name):
        filename(name),
        file_size(0ull),
        stream(nullptr) {
        symlinkname = filename;
        symlinkname += ".log";
        /* Try to create the file when Initalization. */
        _createFile();
    }
    ~FileWritter() {
        if (stream) {
            fflush(stream);
            fsync(fd);
            clearAllPageCache();
        }
    }
    bool write(const char *data, int len) {
        /* If create file failed last time, try to do it every call. */
        if (!stream) _createFile();
        if (!stream) return false;
        
        // fwrite() doesn't return an error when the disk is full, for
        // messages that are less than 4096 bytes. When the disk is full,
        // it returns the message length for messages that are less than
        // 4096 bytes. fwrite() returns 4096 for message lengths that are
        // greater than 4096, thereby indicating an error.
        fwrite(data, 1, len, stream);
        if (errno == ENOSPC) {
            // disk full, stop writing to disk
            return false;
        }
        file_size += len;
        return true;
    }
    void clearPageCache() {
        if (file_size >= PAGE_SIZE) {
            uint32_t len = (file_size) & ~(PAGE_SIZE - 1);
            posix_fadvise(fileno(stream), 0, len, POSIX_FADV_DONTNEED);
        }
    }
    /* When deconstruct the logWritter or rotate the log, could clear all the file's page cache. */
    void clearAllPageCache() {
        uint32_t len = (file_size) & ~(PAGE_SIZE);
        posix_fadvise(fileno(stream), 0, len, POSIX_FADV_DONTNEED);
    }
    void flush() {
        if (!stream) return;
        
        fflush(stream);
        fdatasync(fd);
    }
    /* Rotate the log file. */
    void rotate() {
        if (filename.empty()) return;
        if (stream) {
            fclose(stream);
            fd = -1;
            stream = NULL;
        }

        _createFile();
    }
    uint64_t fileSize() const {
        return file_size;
    }
private:
    string filename;
    string symlinkname;
    uint64_t file_size;
    int fd;
    FILE *stream;
    
    bool _createFile() {
        file_size = 0;
        if (filename.empty()) {
            stream = stdout;
            return true;
        }
        if (stream) {
            fclose(stream);
            fd = -1;
            stream = NULL;
        }
        const char *name = filename.c_str();
        struct tm tm_time;
        formatUnixTime(&tm_time);
        
        ostringstream new_filename_stream;
        new_filename_stream.fill('0');
        new_filename_stream << filename
                        << '.'
                        << 1900 + tm_time.tm_year
                        << setw(2) << 1 + tm_time.tm_mon
                        << setw(2) << tm_time.tm_mday
                        << '-'
                        << setw(2) << tm_time.tm_hour
                        << setw(2) << tm_time.tm_min
                        << setw(2) << tm_time.tm_sec
                        << '.'
                        << getpid();
        const string& new_filename = new_filename_stream.str();

        fd = open(new_filename.c_str(), O_WRONLY | O_CREAT | O_EXCL, 0664);
        if (fd == -1) {
            fprintf(stderr, "COULD NOT CREATE LOGFILE '%s'!\n", new_filename.c_str());
            return false;
        }
        fcntl(fd, F_SETFD, FD_CLOEXEC);  // Mark the file close-on-exec. We don't really care if this fails
        stream = fdopen(fd, "a");
        if (stream == NULL) {
            fprintf(stderr, "COULD NOT CREATE LOGFILE '%s'!\n", new_filename.c_str());
            close(fd);
            unlink(new_filename.c_str());  // Erase the half-baked evidence: an unusable log file
            return false;
        }
        
        /* 比较绕，1，需要创建这个link，是绝对路径
         * 2, 创建和文件之间的关系，是相对路径 */
        const char *slash = strrchr(new_filename.c_str(), '/');
        const char *linkdest = slash ? slash + 1 : new_filename.c_str();
        unlink(symlinkname.c_str());  // delete old one if it exists
        symlink(linkdest, symlinkname.c_str());

        /* Add header content for a new file. */
        ostringstream file_header_stream;
        file_header_stream.fill('0');
        file_header_stream << "Log file created at: "
                        << 1900 + tm_time.tm_year << '/'
                        << setw(2) << 1 + tm_time.tm_mon << '/'
                        << setw(2) << tm_time.tm_mday
                        << ' '
                        << setw(2) << tm_time.tm_hour << ':'
                        << setw(2) << tm_time.tm_min << ':'
                        << setw(2) << tm_time.tm_sec << '\n'
                        << "Running on machine: "
                        << hostname() << '\n'
                        << "Log line format: yy-mm-dd hh:mm:ss.uuuuuu [DIWEF] file:line "
                        << ""
                        << "[file:line thread_role] msg" << '\n';
        const string &file_header_string = file_header_stream.str();
        const int header_len = file_header_string.size();
        fwrite(file_header_string.data(), 1, file_header_string.size(), stream);
        this->file_size += header_len;
        fflush(stream);
        fsync(fd);
        return true;
    }
};

class LogWritter {
public:
    LogWritter(const char *filename) :
        file_writer(filename),
        alloc(),
        mu(),
        cv(),
        loglevel(LOG_INFO),
        sync_loglevel(LOG_FATAL),
        miss_logs(0),
        blocked_threads(0),
        written_log_index(0),
        flushed_log_index(0),
        memtable_size(0),
        disk_full_try_times(0),
        use_color(false),
        max_memtable_size(DEFAULT_MAX_MEMORY_SIZE),
        max_file_size(DEFAULT_MAX_FILE_SIZE) {
        running = true;
        async_thread = thread(&LogWritter::asyncRun, this);
    }
    ~LogWritter() {
        mu.lock();
        running = false;
        cv.notify_all();
        mu.unlock();
        if (async_thread.joinable()) {
            async_thread.join();
        }
    }
    void setLogLevel(int level) {
        lock_guard<mutex> lk(mu);
        loglevel = level;
    }
    void setSyncLoglevel(int level) {
        lock_guard<mutex> lk(mu);
        if (level < loglevel) return;
        
        sync_loglevel = level;
    }
    void setMaxMemtableSize(size_t kb) {
        lock_guard<mutex> lk(mu);
        max_memtable_size = (kb << 10);
    }
    void setFileRotateSize(size_t mb) {
        lock_guard<mutex> lk(mu);
        max_file_size = (mb << 20);
    }
    void setUseColor(bool use) {
        use_color = use;
    }

    void write(bool force_flush, time_t time, char *message, int message_len) {
        unique_lock<mutex> lock(mu);
        
        memtable.push_back(make_pair(message, message_len));
        written_log_index++;
        memtable_size += message_len;

        // must block waiting for disk
        if (force_flush) {
            /* Downgrade to normal log async write, we do not hope waitting for a long time. */
            if (disk_full_try_times > 0) {
                --disk_full_try_times;
                return;
            }
            uint64_t need_flushed_index = written_log_index;
            blocked_threads++;
            cv.notify_all();
            while (need_flushed_index > flushed_log_index) {
                cv.wait(lock);
            }
            blocked_threads--;
            return;
        }
        /* Normal log write. Async write to log. */
        if (memtable_size > 10240) {
            if (disk_full_try_times) {
                --disk_full_try_times;
                return;
            }
            /* The async thread is waiting, notify it. */
            if (blocked_threads > 0) {
                cv.notify_all();
            } else {
                cv.notify_one();
            }
        }
    }

    void writeLog(int loglevel, const char *file, int line, const  char *fmt, va_list ap) {
        unique_lock<mutex> lock(mu);
        if (loglevel < this->loglevel) {
            return;
        }
        bool should_flush = (loglevel >= sync_loglevel);
        lock.unlock();

        /* Check the log thread is busy?
         * If the log thread is busy, the allocator may do not have space. */
        char *buf = static_cast<char *>(alloc.allocate());
        if (buf == nullptr) {
            /* The log is not very importarnt, so just discard it. */
            lock.lock();
            if (loglevel < sync_loglevel) {
                std::cout << "miss log" << std::endl;
                miss_logs++;
                return;
            }
            lock.unlock();
            /* Otherwise wait for serveral times, we do not hope wait forever,
             * maybe for some reason the disk cannot work. */
            for (int i = 0; i < 10; ++i) {
                this_thread::sleep_for(chrono::milliseconds(50));
                buf = static_cast<char *>(alloc.allocate());
                if (buf != nullptr) {
                    break;
                }
                if (buf == nullptr) {
                    lock.lock();
                    miss_logs++;
                    lock.unlock();
                    return;
                }
            }
        }

        /* Format the log's content. */
        int offset = 0;
        int maxlen;
        struct timeval tv;
        struct tm tm_info;
        char *input_buf = buf;
        gettimeofday(&tv, NULL);
        time_t timestamp = static_cast<time_t>(tv.tv_sec + (int)((double)tv.tv_usec * 0.000001));
        localtime_r(&timestamp, &tm_info);
        offset += strftime(buf + offset, maxlen - offset,
                        "%Y-%m-%d %H:%M:%S.", &tm_info);
        offset += snprintf(buf + offset, maxlen - offset, "%06d",
                        (int) tv.tv_usec);
        if (use_color) {
            offset = sprintf(buf, "\033[%dm", logcolors[loglevel]);
            offset += snprintf(buf + offset, maxlen - offset, " [%s]", loglevels[loglevel]);
            offset += snprintf(buf + offset, maxlen - offset, "\33[0m");
        } else {
            offset += snprintf(buf + offset, maxlen - offset, " [%s]", loglevels[loglevel]);
        }
        offset += snprintf(buf + offset, maxlen - offset, " %s:%d ", constBasename(file), line);
        offset += snprintf(buf + offset, maxlen - offset, "%c ", thread_identifier);
        va_list ap_copy;
        va_copy(ap_copy, ap);
        offset += vsnprintf(buf + offset, maxlen - offset, fmt, ap_copy);
        va_end(ap_copy);
        offset += snprintf(buf + offset,  maxlen - offset, "\n");

        write(should_flush, timestamp, buf, offset);
    }

private:
    void asyncRun() {
        char misslog_buf[1024];
        int misslog_len = 0;
        pthread_setname_np(pthread_self(), "async-log");
        pthread_setcancelstate(PTHREAD_CANCEL_ENABLE, NULL);
        pthread_setcanceltype(PTHREAD_CANCEL_ASYNCHRONOUS, NULL);

        unique_lock<mutex> lock(mu);
        while (true) {
            /* Every 10 seconds flush current log data.(maybe the log is still small.) */
            while (running && memtable.empty()) {
                cv.wait_for(lock, chrono::seconds(10));
            }
            /* Wait for serval times for disk availiable again. */
            if (disk_full_try_times > 0 && running) {
                --disk_full_try_times;
                continue;
            }
            /* Break the loop. */
            if (!running && memtable.empty()) {
                break;
            }
            /* Check if exist miss logs. */
            if (miss_logs > 0) {
                int misslog_len = snprintf(misslog_buf, sizeof(miss_logs),
                    "Exceed max log size, miss logs: %d, current mem size: %d", miss_logs, MAX_LOG_BUFFER_SIZE * MAX_LOG_ENTRY);
                miss_logs = 0;
            }
            /* All the file operations should not hold the lock. */
            decltype(memtable) table;
            table.swap(memtable);
            lock.unlock();
            auto ite = table.begin();
            uint64_t current_flushed_logs = 0;
            uint64_t current_written_size = 0;
            for (; ite != table.end(); ++ite) {
                /* If file Exceed max file size, then rotate to another file */
                if (file_writer.fileSize() > max_file_size) {
                    file_writer.clearAllPageCache();
                    file_writer.rotate();
                }

                pair<char *, int> &tmp = *ite;
                if (file_writer.write(tmp.first, tmp.second) == false) {
                    break;
                }
                alloc.deallocate(reinterpret_cast<void *>(tmp.first));
                current_flushed_logs++;
                current_written_size += tmp.second;
            }
            if (misslog_len > 0) {
                file_writer.write(misslog_buf, misslog_len);
                misslog_len = 0;
            }
            file_writer.flush();
            // file_writer.clearPageCache();
            /* Update the metadata for logger. */
            lock.lock();
            if (ite != table.end()) {
                /* Do not write complete, the log write may have some problem.
                 * Wait for serveral seconds. */
                table.erase(table.begin(), ite);
                memtable.splice(memtable.begin(), table);
                /* The attempt times for trying write to disk. */
                disk_full_try_times = 10;
            }
            memtable_size -= current_written_size;
            flushed_log_index += current_flushed_logs;
            if (blocked_threads == 1) {
                cv.notify_one();
            } else if (blocked_threads > 1) {
                cv.notify_all();
            }
        }

        lock.unlock();
        return;
    }

    FileWritter file_writer;
    LoggerAllocator alloc;
    thread async_thread;
    mutex mu;
    condition_variable cv;
    int loglevel;
    int sync_loglevel;
    int miss_logs;
    int blocked_threads;

    uint64_t written_log_index;
    uint64_t flushed_log_index;
    uint64_t memtable_size;
    list<pair<char *, int>> memtable;
    char header;
    bool running;

    /* Disk full failover. */
    int disk_full_try_times;

    /* configurations */
    bool use_color;
    uint64_t max_memtable_size;
    uint64_t max_file_size;
};

static LogWritter *logger;

void initLoggingSystem(const char *filename) {
    logger = new LogWritter(filename);
}

void deinitLoggingSystem() {
    delete logger;
}

void logfunction(int loglevel, const char *file, int line, const char *fmt, ...) {
    va_list ap;
    va_start(ap, fmt);
    logger->writeLog(loglevel, file, line, fmt, ap);
    va_end(ap);
}

void setThreadLocalIdentifier(char identifier) {
    thread_identifier = identifier;
}

char getThreadLocalIdentifier() {
    return thread_identifier;
}

void setLogLevel(int loglevel) {
    logger->setLogLevel(loglevel);
}

void setLogSyncLevel(int loglevel) {
    logger->setSyncLoglevel(loglevel);
}

void setMaxMemorySize(size_t kb) {
    logger->setMaxMemtableSize(kb);
}

void setMaxFileSize(size_t mb) {
    logger->setFileRotateSize(mb);
}

void setLogColor(bool set_color) {
    logger->setUseColor(set_color);
}
