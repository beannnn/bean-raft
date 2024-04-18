
#include "logger.hpp"

#include <stdarg.h>
#include <unistd.h>
#include <string.h>
#include <fcntl.h>
#include <time.h>

#include <thread>
#include <condition_variable>
#include <mutex>
#include <string>
#include <list>
#include <iostream>
#include <sstream>

using namespace std;

static constexpr int LOG_HEADER_SIZE = 32;
static constexpr int MAX_LOG_BUFFER_SIZE = 4096 - 32;  /* max size for normal log */
static constexpr int MAX_LOG_ENTRY = 1024;             /* max memory buffer */
static constexpr int PAGE_SIZE = 4096;                  /* linux file pagecache size */

int64 CycleClock_Now() {
    // TODO(hamaji): temporary impementation - it might be too slow.
    struct timeval tv;
    gettimeofday(&tv, NULL);
    return static_cast<int64>(tv.tv_sec) * 1000000 + tv.tv_usec;
}

static void getFormatTime() {
    
}

// int64 Seconds_Now() {
//     struct timeval tv;
//     gettimeofday(&tv, NULL);
//     return static_cast<int64>(tv.tv_sec) + tv.tv_usec * 0.000001;
// }

// WallTime WallTime_Now() {
//   // Now, cycle clock is retuning microseconds since the epoch.
//   return CycleClock_Now() * 0.000001;
//   stru
// }


    // struct ::tm tm_time;
    // time_t timestamp = static_cast<time_t>(WallTime_Now());
    // memset(&tm_time, 0, sizeof(tm_time));
    // localtime_r(&timestamp, &tm_time);


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
    // 前面包装一个type 表示是否超过了大小.
    // 分配的时候提前计算type，后续deallocate的时候决定是直接使用还是用raw.
    void *allocate(int is_raw) {
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

// append only.
// 每个log有一个单独的index.
// 

// file的操作是单独线程去做的。
// 这里操作rotate这些
// 记录写入的大小等等。
// write触发或者时间触发重建。
class FileWritter {
public:
    FileWritter(const char *filename):filename(filename) {
        
    }
    /* 不管是磁盘满了，还是rotate创建文件失败了，都会重新走到这个函数中。如果失败了应该是以一定的策略进行重试。 */
    bool write(const char *data, int len) {
        if (!filename.empty() && !stream) _createFile();
        if (!stream) return false;
        
        // fwrite() doesn't return an error when the disk is full, for
        // messages that are less than 4096 bytes. When the disk is full,
        // it returns the message length for messages that are less than
        // 4096 bytes. fwrite() returns 4096 for message lengths that are
        // greater than 4096, thereby indicating an error.
        fwrite(data, 1, len, stream);
        if (errno == ENOSPC) {
            // disk full, stop writing to disk
            write_error = true;
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
    /* 当发生文件rotate的时候，需要 */
    void clearAllPageCache() {
        uint32_t len = (file_size) & ~(PAGE_SIZE);
        posix_fadvise(fileno(stream), 0, len, POSIX_FADV_DONTNEED);
    }
    void flush() {
        
    }
    /* 清理当前的stream的资源 */
    void rotate() {
        if (filename.empty()) return;
        if (stream) {
            fclose(stream);
            stream = NULL;
        }

        _createFile();
    }
    uint64_t fileSize() const {
        return file_size;
    }
private:
    string filename;
    uint64_t file_size;
    FILE *stream;
    bool write_error;
    
    bool _createFile() {
        file_size = 0;
        if (filename.empty()) {
            stream = stdout;
        } else {
            if (stream) {
                fclose(stream);
                stream = NULL;
            }
            const char *name = filename.c_str();
            // localtime_s();
            // time_t tm_time;
            ostringstream time_pid_stream;
            time_pid_stream.fill('0');
            // time_pid_stream << 1900 + tm_time.tm_year
            //                 << setw(2) << 1 + tm_time.tm_mon
            //                 << setw(2) << tm_time.tm_mday
            //                 << '-'
            //                 << setw(2) << tm_time.tm_hour
            //                 << setw(2) << tm_time.tm_min
            //                 << setw(2) << tm_time.tm_sec
            //                 << '.'
            //                 << GetMainThreadPid();
            // const string& time_pid_string = time_pid_stream.str();
            // stream = fopen(filename.c_str(), "a");
            // fprintf(stderr, "COULD NOT CREATE LOGFILE '%s'!\n",
            //     time_pid_string.c_str());

            int fd = open(name, O_WRONLY | O_CREAT | O_EXCL, 0664);
            if (fd == -1) return false;
            
            // 失败了这个应该是后续继续尝试创建
            stream = fdopen(fd, "a");
            if (stream == NULL) {
                close(fd);
                // unlink(stream);
                return false;
            }
            
            const char* slash = strchr(name, '/');
            string linkname = string(name) + ".log";
            string linkpath;
            if ( slash ) linkpath = string(name, slash - name + 1);  // get dirname
            linkpath += linkname;
            unlink(linkpath.c_str());                    // delete old one if it exists
            const char *linkdest = slash ? (slash + 1) : name;
            symlink(linkdest, linkpath.c_str());

            // 还有file header的问题
    //             ostringstream file_header_stream;
    // file_header_stream.fill('0');
    // file_header_stream << "Log file created at: "
    //                    << 1900+tm_time.tm_year << '/'
    //                    << setw(2) << 1+tm_time.tm_mon << '/'
    //                    << setw(2) << tm_time.tm_mday
    //                    << ' '
    //                    << setw(2) << tm_time.tm_hour << ':'
    //                    << setw(2) << tm_time.tm_min << ':'
    //                    << setw(2) << tm_time.tm_sec << '\n'
    //                    << "Running on machine: "
    //                    << LogDestination::hostname() << '\n'
    //                    << "Log line format: [IWEF]yy-mm-dd hh:mm:ss.uuuuuu "
    //                    << "threadid file:line] msg" << '\n';
    // const string& file_header_string = file_header_stream.str();
    // const int header_len = file_header_string.size();
    // // write to the file header
    // fwrite(file_header_string.data(), 1, header_len, file_);
    // file_length_ += header_len;
    // fflush(file_);
        }
    }
};

class LogWritter {
public:
    LogWritter(const char *filename) : file_writer(filename) {
        written_log_index = 0;
        flushed_log_index = 0;
        memtable_size = 0;
        
        max_memtable_size = 0;
        max_file_size = 0;
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
    void setLogLevel(int level) {
        lock_guard<mutex> lk(mu);
        loglevel = level;
    }

    // format the log.
    // 有的东西是需要一开始就定义好的。
    // 同时需要
    /* 落盘相关的单独用一个class */
    void write(int loglevel, time_t time, const char *message, int message_len) {
        unique_lock<mutex> lock;

        // if 
        if (loglevel < loglevel) {
            mu.unlock();
            return;
        }
        
        memtable.push_back(make_pair(message, message_len));
        written_log_index++;
        memtable_size += message_len;
        // int64 now = 

        // must block waiting for disk
        if (loglevel >= sync_loglevel) {
            uint64_t need_flushed_index = written_log_index;
            blocked_threads++;
            while (need_flushed_index > flushed_log_index) {
                cv.wait(lock);
            }
            blocked_thread--;
            return;
        }
        /* Normal case. */
        // int64 now = 
        if (memtable_size > 10240) {
            cv.notify();
        }
    }
private:
    /* 这个函数不依赖与任何, 是否需要考虑写入失败的情况，使用磁盘我理解是必须要考虑的。是否需要重新拼接。 */
    void asyncRun() {
        pthread_setcancelstate(PTHREAD_CANCEL_ENABLE, NULL);
        pthread_setcanceltype(PTHREAD_CANCEL_ASYNCHRONOUS, NULL);
        unique_lock<mutex> lock(mu);
        lock.lock();
        
        while (true) {
            while (running && !memtable.empty()) {
                cv.wait(lock);
            }
            /* Stop the loop, break it. */
            if (!running && memtable.empty()) {
                break;
            }

            /* 尝试进行flush，这里不需要持有锁 */
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

                pair<const char *, int> &tmp = *ite;
                if (file_writer.write(tmp.first, tmp.second) == false) {
                    break;
                }
                current_flushed_logs++;
                current_written_size += tmp.second;
            }
            file_writer.flush();
            file_writer.clearPageCache();
            
            lock.lock();
            if (ite != table.end()) {
                /* Do not write complete, the log write may have some problem.
                 * Wait for serveral seconds. */
                table.erase(table.begin(), ite);
                memtable.splice(memtable.begin(), table);
                // stop_write = true;
            }
            memtable_size -= current_written_size;
            // next_flush_time = ;
            flushed_log_index += current_flushed_logs;
            
            if (blocked_threads == 1) {
                
            } else if (blocked_threads > 1) {
                
            }
        }

        lock.unlock();
    }

    FileWritter file_writer;
    mutex mu;
    condition_variable cv;
    int sync_loglevel;
    int loglevel;
    int miss_logs;
    int blocked_threads;

    uint64_t written_log_index;
    uint64_t flushed_log_index;
    uint64_t memtable_size;
    uint64_t max_memtable_size;
    uint64_t max_file_size;
    list<pair<const char *, int>> memtable;
    int64_t next_flush_time;
    char header;
    bool running;
};

static LogWritter *logger;
static LoggerAllocator alloc;

void initLoggingSystem(const char *filename) {
    // logger = new LogWritter(filename);
}

void deinitLoggingSystem() {
    // delete logger;
}


//   class GOOGLE_GLOG_DLL_DECL LogStream : public std::ostream {
// #ifdef _MSC_VER
// # pragma warning(default: 4275)
// #endif
//   public:
//     LogStream(char *buf, int len, int ctr)
//         : std::ostream(NULL),
//           streambuf_(buf, len),
//           ctr_(ctr),
//           self_(this) {
//       rdbuf(&streambuf_);
//     }

//     int ctr() const { return ctr_; }
//     void set_ctr(int ctr) { ctr_ = ctr; }
//     LogStream* self() const { return self_; }

//     // Legacy std::streambuf methods.
//     size_t pcount() const { return streambuf_.pcount(); }
//     char* pbase() const { return streambuf_.pbase(); }
//     char* str() const { return pbase(); }

// format 不是特别好写，要不还是按照C的方式实现。

static const char* constBasename(const char* filepath) {
  const char* base = strrchr(filepath, '/');
  return base ? (base+1) : filepath;
}

static void formatLog(int is_raw, int loglevel, const char *file, int line) {
    int offset = 0;
    int maxlen;

    char *buf = static_cast<char *>(alloc.allocate());
    if (buf == nullptr) {
        /* Depend on logleve, buf we do not block forever. */
        
    }

    
    if (!is_raw) {
        struct timeval tv;
        struct tm tm_info;
        char *input_buf = buf;
        gettimeofday(&tv, NULL);
        // 这样计算是不是存在问题。
        time_t timestamp = static_cast<time_t>(tv.seconds + (int)((double)tv.useconds * 0.000001));
        localtime_r(&timestamp, &tm_info);
        offset += strftime(buf + offset, maxlen - offset,
                           "%Y-%m-%d %H:%M:%S.", &tm_info);
        offset += snprintf(buf + offset, maxlen - offset, "%06d",
                           (int) tv.tv_usec);
        // [ERROR]
        // [FATAL]
        offset += snprintf(buf + offset, maxlen - offset, " %s:%d ", constBasename(file), line);
        // [M]
    } else {
        // raw使用外部的Slice作为参数.
        // 如何知道呢？
        // 我们free的时候提前把header暴露出来
        // 前面的内容就是type就可以了。
    }
    
}

/* 这里进行format */
// header之后增加一个role的参数，实现应该是thread local的, 然后其他的线程来进行设置。
void logfunction(int loglevel, const char *file, int line, const char *fmt, ...) {
    
    

    // format the log here.
    // 使用c++的stream来做。
    // 还可以比较方便地使用stream
    // 
//   if (FLAGS_log_prefix && (line != kNoLogPrefix)) {
//     stream() << setw(4)<< 1900+data_->tm_time_.tm_year << "-"
//              << setw(2) << 1+data_->tm_time_.tm_mon << "-"
//              << setw(2) << data_->tm_time_.tm_mday
//              << ' '
//              << setw(2) << data_->tm_time_.tm_hour  << ':'
//              << setw(2) << data_->tm_time_.tm_min   << ':'
//              << setw(2) << data_->tm_time_.tm_sec   << "."
//              << setw(6) << usecs << ' '
//              << "[" << LogSeverityNames[severity][0] << "] "
//              << setfill(' ') << setw(5)
//              << static_cast<unsigned int>(GetTID()) << setfill('0')
//              << ' '
//              << data_->basename_ << ':' << data_->line_ << ' ';
//   }
//   data_->num_prefix_chars_ = data_->stream_.pcount();
    // format的时候需要生成一个时间
    // 我都还没有format
    // 有一个问题是，这个是一个整体的，只能通过macro来自定义一些header.
}


// #define LOG_BUF_LENGTH  2048  // free-list的大小
// #define LOG_LENGTH      1024  // free-list的长度

// #define SDS_ENCODE  0
// #define CHAR_ENCODE 1

// static FILE *log_stream = NULL;
// static int log_running = 0;
// static pthread_t log_thread_id = 0;
// static int miss_logs = 0;
// static unsigned long long current_log_index;
// static unsigned long long writen_log_index;
// static int blocked_threads;
// static int sync_log_level;  // >= 该日志等级的日志都会同步持久化，否则异步持久化
// static pthread_mutex_t log_mutex;
// static pthread_cond_t log_cond;
// static list *log_free_list = NULL;
// static list *log_list = NULL;

// int getSyncLogLevel() {
//     int ret;
//     pthread_mutex_lock(&log_mutex);
//     ret = sync_log_level;
//     pthread_mutex_unlock(&log_mutex);
//     return ret;
// }

// void updateSyncLogLevel(int level) {
//     pthread_mutex_lock(&log_mutex);
//     sync_log_level = level;
//     pthread_mutex_unlock(&log_mutex);
// }

// void *logRun(void *arg) {
//     (void)arg;
//     pthread_setname_np(pthread_self(), "Proxy-Log");
//     list *log_tmp = listCreate();
//     while (1) {
//         pthread_mutex_lock(&log_mutex);
//         while (log_running && listLength(log_list) == 0 && miss_logs == 0) {
//             pthread_cond_wait(&log_cond, &log_mutex);
//         }
//         if (!log_running && listLength(log_list) == 0) {
//             break;
//         }
//         listJoin(log_tmp, log_list);
//         int miss_logs_tmp = miss_logs;
//         miss_logs = 0;
//         pthread_mutex_unlock(&log_mutex);
//         listIter li;
//         listNode *ln;
//         /* 不考虑日志落盘失败情况 */
//         for (listRewind(log_tmp, &li); ((ln = listNext(&li)) != NULL);) {
//             logEntry *entry = (logEntry *) ln->value;
//             if (entry->sds_buf) {
//                 fprintf(log_stream, "%s\n", (const char *) entry->sds_buf);
//                 sdsfree(entry->sds_buf);
//                 entry->sds_buf = NULL;
//             } else {
//                 fprintf(log_stream, "%s\n", entry->buf);
//             }
//             writen_log_index++;
//         }
//         if (miss_logs_tmp > 0) {
//             fprintf(log_stream, "[Proxy-Log] logs writting exceed memory limlit, missed log: %d\n", miss_logs_tmp);
//         }
//         fflush(log_stream);
//         /* 重新加锁更新一些数据 */
//         pthread_mutex_lock(&log_mutex);
//         listJoin(log_free_list, log_tmp);
//         if (blocked_threads > 0) {
//             pthread_cond_broadcast(&log_cond);
//         }
//         pthread_mutex_unlock(&log_mutex);
//     }
//     return NULL;
// }

// /* 如果初始化失败返回0 */
// int initLog() {
//     if (config.logfile != NULL) {
//         log_stream = fopen(config.logfile, "a");
//         if (log_stream == NULL) {
//             return 0;
//         }
//     } else {
//         log_stream = stdout;
//     }
//     sync_log_level = LOGLEVEL_ERROR;
//     blocked_threads = 0;
//     pthread_mutexattr_t attr;
//     pthread_mutexattr_init(&attr);
//     pthread_mutexattr_settype(&attr, PTHREAD_MUTEX_RECURSIVE);
//     pthread_mutex_init(&log_mutex, &attr);
//     pthread_mutexattr_destroy(&attr);
//     pthread_cond_init(&log_cond, NULL);
//     log_running = 1;
//     log_list = listCreate();
//     log_free_list = listCreate();
//     /* 预分配log内存，通过free-list进行链接 */
//     for (int i = 0; i < LOG_LENGTH; ++i) {
//         logEntry *entry = zmalloc(sizeof(*entry));
//         entry->buf = zmalloc(LOG_BUF_LENGTH);
//         entry->sds_buf = NULL;
//         listAddNodeTail(log_free_list, entry);
//     }
//     if (pthread_create(&log_thread_id, NULL, logRun, NULL)) {
//         return 0;
//     }
//     return 1;
// }

// void deinitLog() {
//     pthread_mutex_lock(&log_mutex);
//     log_running = 0;
//     pthread_cond_signal(&log_cond);
//     pthread_mutex_unlock(&log_mutex);
//     if (log_thread_id != 0) {
//         if (pthread_join(log_thread_id, NULL) != 0) {
//             fprintf(log_stream, "pthread join failed");
//             exit(1);
//         }
//     }
//     if (config.logfile != NULL) fclose(log_stream);
//     pthread_cond_destroy(&log_cond);
//     pthread_mutex_destroy(&log_mutex);
//     assert(log_list != NULL && listLength(log_list) == 0);
//     listRelease(log_list);
//     if (log_free_list) {
//         listIter li;
//         listNode *ln;
//         for (listRewind(log_free_list, &li); ((ln = listNext(&li)) != NULL); ) {
//             logEntry *entry = ln->value;
//             assert(entry->buf != NULL);
//             zfree(entry->buf);
//             zfree(entry);
//         }
//         listRelease(log_free_list);
//     }
// }

// sds formatLog(char *buf, size_t buf_len, const char *file, int line, int level, const char *format, va_list ap);
// void proxyLog(const char *file, int line, int level, const char *format, ...) {
//     if (level < config.loglevel) return;
//     pthread_mutex_lock(&log_mutex);
//     if (!log_running) {
//         pthread_mutex_unlock(&log_mutex);
//         return;
//     } else if (listLength(log_free_list) == 0) {
//         miss_logs++;
//         pthread_cond_broadcast(&log_cond);
//         pthread_mutex_unlock(&log_mutex);
//         return;
//     }
//     unsigned long long my_log_index = current_log_index++; // 在当前这个index中写入这条日志，同时记录日志的索引
//     // 从free-list队列中pop一个元素
//     logEntry *entry = listFirst(log_free_list)->value;
//     listDelNode(log_free_list, listFirst(log_free_list));
//     va_list args;
//     va_start(args, format);
//     sds tmp = formatLog(entry->buf, LOG_BUF_LENGTH, file, line, level, format, args);
//     va_end(args);
//     if (tmp) {
//         entry->sds_buf = tmp;
//     }
//     listAddNodeTail(log_list, entry);
//     pthread_cond_broadcast(&log_cond);
//     if (level >= sync_log_level) {
//         blocked_threads++;
//         while (my_log_index > writen_log_index) {
//             pthread_cond_wait(&log_cond, &log_mutex);
//         }
//         blocked_threads--;
//     }
//     pthread_mutex_unlock(&log_mutex);
// }

// const char* constBasename(const char* filepath) {
//   const char* base = strrchr(filepath, '/');
//   return base ? (base+1) : filepath;
// }

// /* 如果日志buf长度不足够，同时这条日志必须输出完整信息，那么返回一个新临时创建的sds字符串 */
// sds formatLog(char *buf, size_t buf_len, const char *file, int line, int level, const char *format, va_list ap) {
//     int is_raw = 0, use_colors = config.use_colors;
//     if ((is_raw = (level & LOG_RAW))) {
//         level &= ~(LOG_RAW);
//         use_colors = 0;
//     }
//     va_list ap_backup;
//     va_copy(ap_backup, ap);
//     time_t t;
//     struct timeval tv;
//     struct tm tm_info;
//     int offset = 0, before_format_offset;
//     size_t maxlen = buf_len - 1;
//     char *input_buf = buf;
//     gettimeofday(&tv, NULL);
//     time(&t);
//     localtime_r(&t, &tm_info);
//     if (use_colors) {
//         int color = LOG_COLOR_DEFAULT;
//         switch (level) {
//         case LOGLEVEL_DEBUG: color = LOG_COLOR_GRAY; break;
//         case LOGLEVEL_INFO: color = LOG_COLOR_DEFAULT; break;
//         case LOGLEVEL_SUCCESS: color = LOG_COLOR_GREEN; break;
//         case LOGLEVEL_WARNING: color = LOG_COLOR_YELLOW; break;
//         case LOGLEVEL_ERROR: color = LOG_COLOR_RED; break;
//         }
//         offset = sprintf(buf, "\033[%dm", color);
//         maxlen -= 4; /* Keep enough space for final "\33[0m"  */
//     }
//     if (!is_raw) {
//         int thread_id = getCurrentThreadID();
//         offset += strftime(buf + offset, maxlen - offset,
//                            "[%Y-%m-%d %H:%M:%S.", &tm_info);
//         offset += snprintf(buf + offset, maxlen - offset, "%03d",
//                            (int) tv.tv_usec/1000);
//         offset += snprintf(buf + offset, maxlen - offset, " %s:%d", constBasename(file), line);
//         if (thread_id != PROXY_MAIN_THREAD_ID) {
//             offset += snprintf(buf + offset, maxlen - offset, "/%d] ",
//                 thread_id);
//         } else {
//             offset += snprintf(buf + offset, maxlen - offset, "/M] ");
//         }
//     }
//     before_format_offset = offset;
//     offset += vsnprintf(buf + offset, maxlen - offset, format, ap);
//     // 有时候不希望遗漏信息，所以会增加buf的大小
//     // 这种情况下，可能需要append的方法比较好，可以使用Slice的
//     if ((is_raw || level == LOGLEVEL_DEBUG) && (size_t) offset >= maxlen) {
//         offset = before_format_offset;
//         buf = sdsnewlen(buf, offset);
//         buf = sdscatvprintf(buf, format, ap_backup);
//         if (use_colors) buf = sdscat(buf, "\33[0m");
//     } else if (use_colors) {
//         snprintf(buf + offset, maxlen - offset, "\33[0m");
//     }
//     va_end(ap_backup);
//     return buf == input_buf ? NULL : buf;
// }


