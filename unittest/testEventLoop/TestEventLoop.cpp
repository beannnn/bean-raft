#include "gtest/gtest.h"
#include "gmock/gmock.h"

#include "eventloop.hpp"

#include <iostream>

class EventloopTest : public testing::TestWithParam<int> {
protected:
    virtual void SetUp() override {
    }
    virtual void TearDown() override {
    }
};

TEST(EventloopTest, basicNewAndDelete) {
    EventLoop *el = new EventLoop();
    bool ret = el->init();
    ASSERT_EQ(ret, true);
    delete el;
}

// 对于连接我们可以设置任何的可能性。
// 比如设置一个节点的
// connections的一些监控。

// TEST_F(AETest, epollSyscall) {
//     MOCKER(epoll_create).expects(once()).will(returnValue(10));
//     MOCKER(epoll_ctl).expects(once()).will(returnValue(0));
//     MOCKER(epoll_wait).expects(once()).will(returnValue(0));
//     aeEventLoop *ae = aeCreateEventLoop(100);
//     int res = aeCreateFileEvent(ae, 1, AE_READABLE, NULL, NULL);
//     res = aeProcessEvents(ae, AE_ALL_EVENTS);
//     aeDeleteEventLoop(ae);
// }

// https://blog.csdn.net/just_lion/article/details/102730698
// https://blog.csdn.net/xueyong4712816/article/details/34086649
// 因为我们需要对系统的函数进行mock的，对类呢？
// SystemBeginMock();
// EL, network.

// mock all the items.
// 怎么办, 要不要还是使用mockcpp.
// gmock不太好用啊
// 要不要用mockcpp.

// TEST_F(AETest, createFileEvent) {
//     MOCKER(epoll_ctl).stubs().will(returnValue(0));
//     aeEventLoop *ae = aeCreateEventLoop(200);
//     for (int i = 0; i < 200; i++) {
//         int res = aeCreateFileEvent(ae, i, AE_READABLE, NULL, NULL);
//         ASSERT_EQ(res, 0);
//     }
//     ASSERT_NE(0, aeCreateFileEvent(ae, 200, AE_READABLE, NULL, NULL));
//     ASSERT_NE(0, aeCreateFileEvent(ae, 201, AE_READABLE, NULL, NULL));
//     // extend the size of events
//     ASSERT_EQ(AE_OK, aeResizeSetSize(ae, 400));
//     for (int i = 200; i < 400; ++i) {
//         int res = aeCreateFileEvent(ae, i, AE_READABLE, NULL, NULL);
//         ASSERT_EQ(res ,0);
//     }
//     aeDeleteEventLoop(ae);
// }

// TEST_F(AETest, createTimeEvent) {
//     MOCKER(epoll_ctl).stubs().will(returnValue(0));
//     aeEventLoop *ae = aeCreateEventLoop(200);
//     std::vector<int> timeData(10);
//     for (int i = 0; i < 10; ++i) timeData[i] = i;
//     for (int i = 0; i < 10; ++i) aeCreateTimeEvent(ae, 50 * (i + 1), timeEventFunc, &timeData[i], NULL);
//     // sleep 700 milliseconds.
//     usleep(700000);
//     aeProcessEvents(ae, AE_ALL_EVENTS);
//     for (int i = 0; i < 10; ++i) ASSERT_EQ(0, timeData[i]);
//     aeDeleteEventLoop(ae);
// }
