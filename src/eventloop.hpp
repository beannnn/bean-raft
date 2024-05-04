#ifndef __BEAN_EVENTLOOP_H__
#define __BEAN_EVENTLOOP_H__

#include <functional>
#include <queue>
#include <vector>
#include <sys/epoll.h>

#define EPOLLIN  (1 << 1)
#define EPOLLOUT (1 << 2)

#define EL_OK  1
#define EL_ERR 0

#define EL_NONE     00
#define EL_READABLE 1
#define EL_WRITABLE 2

class EventLoop;

struct fileEvent {
    fileEvent() {
        mask = EL_NONE;
        readfn = nullptr;
        writefn = nullptr;
        fd = -1;
        client_data = NULL;
    }
    fileEvent(const fileEvent &) = default;
    fileEvent &operator=(const fileEvent &) = default;
    ~fileEvent() = default;
    int mask;  /* one of EL_(READABLE|WRITABLE|BARRIER) */
    std::function<void(EventLoop *, int, void *)> readfn;
    std::function<void(EventLoop *, int, void *)> writefn;
    int fd;
    void *client_data;
};

struct timeEvent {
    long long when;  /* expire UNIX time */
    std::function<int(EventLoop *, void *)> fn;  /* timed function */
    void *client_data;
};

struct timeEventCompare {
    bool operator()(const timeEvent &t1, const timeEvent &t2) {
        return t1.when < t2.when;
    }
};

class EventLoop {
public:
    EventLoop();
    ~EventLoop() = default;
    EventLoop(const EventLoop&) = delete;
    EventLoop& operator=(const EventLoop&) = delete;
    bool init();
    void addTimerEvent(long long millisecond, void *client_data, const std::function<int(EventLoop *, void *)> &fn);
    int addFileEvent(int fd, int mask, void *data, const std::function<void(EventLoop *, int, void*)> &fn);
    void delFileEvent(int fd, int mask);
    void setBeforeSleep(const std::function<int(void *)> &f, void *data);
    void setAfterSleep(const std::function<int(void *)> &f, void *data);
    void processEvents();
    void polling();
    void stop();
private:
    int mstime(void);
    void processTimeEvents();
private:
    /* record the absolute time */
    std::priority_queue<timeEvent, std::vector<timeEvent>, timeEventCompare> timer_events;
    std::function<int(EventLoop *)> before_sleep;
    std::function<int(EventLoop *)> after_sleep;
    time_t last_time;
    /* linux implementation for polling */
    int epollfd;
    std::vector<struct epoll_event> epoll_events;
    std::vector<fileEvent> events;
    /* eventloop state */
    bool running;
    /* private data for this eventloop */
public:
    void *private_data;
};

#endif // __BEAN_EVENTLOOP_H__
