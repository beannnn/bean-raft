

#include <queue>
#include <functional>
#include <vector>
#include <cassert>

#include <sys/time.h>

#include "eventloop.hpp"

using namespace std;

EventLoop::EventLoop() {
    before_sleep = nullptr;
    after_sleep = nullptr;
    running = true;
    epollfd = -1;
}

bool EventLoop::init() {
    epollfd = epoll_create(1024); /* 1024 is just a hint for the kernel */
    if (epollfd == -1) {
        return false;
    }
    return true;
}

void EventLoop::addTimerEvent(long long millisecond, void *client_data, const std::function<int(EventLoop *, void *)> &fn) {
    int current_mstime = mstime();
    timeEvent event;
    event.client_data = client_data;
    event.fn = fn;
    event.when = current_mstime;
    timer_events.emplace(std::move(event));
}

int EventLoop::addFileEvent(int fd, int mask, void *data, const std::function<void(EventLoop *, int, void*)> &fn) {
    /* should resize the events vector, resize will init all the events */
    if (fd >= events.size()) {
        int newsize = fd > events.size() * 2 ? fd : events.size() * 2;
        events.resize(newsize);
    }
    
    fileEvent &event = events[fd];
    struct epoll_event ee = {0}; /* avoid valgrind warning */
    int op = (event.mask == EL_NONE ? EPOLL_CTL_ADD : EPOLL_CTL_MOD);  /* check first init in epoll or not. */
    
    ee.events = 0;
    if (mask & EL_READABLE) {
        ee.events |= EPOLLIN;
        event.readfn = fn;
    }
    if (mask & EL_WRITABLE) {
        ee.events |= EPOLLOUT;
        event.writefn = fn;
    }
    ee.data.fd = fd;
    if (epoll_ctl(epollfd, op, fd, &ee) == -1) return EL_ERR;
    return EL_OK;
}

void EventLoop::delFileEvent(int fd, int mask) {
    if (fd > events.size()) return;
    fileEvent &event = events[fd];
    if (event.mask == EL_NONE) return;

    int newmask = event.mask & (~mask);
    struct epoll_event ee = {0}; /* avoid valgrind warning */
    ee.events = 0;
    if (newmask & EL_READABLE) ee.events |= EPOLLIN;
    if (newmask & EL_WRITABLE) ee.events |= EPOLLOUT;
    if (newmask != EL_NONE) {
        epoll_ctl(epollfd, EPOLL_CTL_MOD, fd, &ee);
    } else {
        epoll_ctl(epollfd, EPOLL_CTL_DEL, fd, &ee);
    }
}

void EventLoop::setBeforeSleep(const std::function<int(void *)> &f, void *data) {
    before_sleep = f;
}

void EventLoop::setAfterSleep(const std::function<int(void *)> &f, void *data) {
    after_sleep = f;
}

void EventLoop::processEvents() {
    /* count the wait time. */
    int retval = epoll_wait(epollfd, epoll_events.data(), events.size(), -1);
    /* process after sleep. */
    if (after_sleep) {
        after_sleep(this);
    }
    
    if (retval > 0) {
        /* epoll triggle. */
        for (int i = 0; i < retval; ++i) {
            struct epoll_event *e = epoll_events.data() + i;
            int mask = 0;
            if (e->events & EPOLLIN) mask |= EL_READABLE;
            if (e->events & EPOLLOUT) mask |= EL_WRITABLE;
            if (e->events & EPOLLERR) mask |= EL_WRITABLE;
            if (e->events & EPOLLHUP) mask |= EL_WRITABLE;

            assert(e->data.fd < events.size());
            fileEvent &event = events[e->data.fd];
            /* process the events */
            if ((mask & EL_READABLE)) {
                event.readfn(this, e->data.fd, event.client_data);
            }
            if ((mask & EL_WRITABLE)) {
                event.writefn(this, e->data.fd, event.client_data);
            }
        }
    }

    /* Process time functions */
    processTimeEvents();
}

/* Step into polling loop. */
void EventLoop::polling() {
    running = true;
    while (running) {
        if (before_sleep) {
            before_sleep(this);
        }
    }
}

void EventLoop::stop() {
    running = false;
}

int EventLoop::mstime(void) {
    struct timeval tv;
    int mst;

    gettimeofday(&tv, NULL);
    mst = (static_cast<int>(tv.tv_sec)) * 1000;
    mst += static_cast<int>(tv.tv_usec / 1000);
    return mst;
}

void EventLoop::processTimeEvents() {
    /* If the system clock is moved to the future, and then set back to the
    * right value, time events may be delayed in a random way. Often this
    * means that scheduled operations will not be performed soon enough.
    *
    * Here we try to detect system clock skews, and force all the time
    * events to be processed ASAP when this happens: the idea is that
    * processing events earlier is less dangerous than delaying them
    * indefinitely, and practice suggests it is. */
    time_t now = time(NULL);
    if (now < last_time) {
        priority_queue<timeEvent, vector<timeEvent>, timeEventCompare> new_timer_events;
        while (!timer_events.empty()) {
            timeEvent event = timer_events.top();
            
            int ret = event.fn(this, event.client_data);
            if (ret != -1) {
                new_timer_events.emplace(std::move(event));
            }
        }
        timer_events.swap(new_timer_events);
        return;
    }
    last_time = now;

    /* Internal use ms time to check if expired. */
    int current_mstime = mstime();
    while (!timer_events.empty() && timer_events.top().when <= current_mstime) {
        timeEvent event = timer_events.top();
        timer_events.pop();
        int ret = event.fn(this, event.client_data);
        current_mstime = mstime();
        if (ret != -1) {
            /* update the time. */
            event.when = current_mstime;
            timer_events.emplace(event);
        }
    }
}
