#ifndef __REDIS_PROXY_EL_H__
#define __REDIS_PROXY_EL_H__

#include <functional>
#include <queue>
#include <vector>


#define EPOLLIN  (1 << 1)
#define EPOLLOUT (1 << 2)

#define EL_OK  1
#define EL_ERR 0

#define EL_NONE     00
#define EL_READABLE 1
#define EL_WRITABLE 2

struct fileEvent;
struct timeEvent;
struct timeEventCompare;

class EventLoop {
public:
    EventLoop();
    ~EventLoop() = default;
    EventLoop(const EventLoop&) = delete;
    EventLoop& operator=(const EventLoop&) = delete;
    void addTimerEvent(long long millisecond, void *client_data, const std::function<int(EventLoop *, void *)> &fn);
    /* TODO: Impleted by time wheel. */
    // void addRoughTimerEvent(long long millisecond, const std::function<int(EventLoop *, void *)> &f);
    int addFileEvent(int fd, int mask, void *data, const std::function<void(EventLoop *, int, void*)> &fn);
    void delFileEvent(int fd, int mask);
    void setBeforeSleep(const std::function<int(void *)> &f, void *data);
    void setAfterSleep(const std::function<int(void *)> &f, void *data);
    void processEvents();
    void polling();
    void stop();
public:
    void *private_data;
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
    /* private data for loop */
    void *private_data;
};

#endif // __REDIS_PROXY_EL_H__
