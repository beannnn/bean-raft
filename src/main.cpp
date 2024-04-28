
#include <iostream>
#include <vector>
#include <chrono>
#include <thread>

#include "slice.hpp"
#include "logger.hpp"

using namespace std;

int main() {
    /* config文件是如何确定的，同时最好抽象一个state machine, kv db. */
    // Slice s("hello slice");
    // s.appendFmt("bbb: %s", "xxx");
    // std::cout << s << endl;

    initLoggingSystem("/home/bean/bean-raft/log");
    // initLoggingSystem("");
    setThreadLocalIdentifier('M');
    logInfo("hello world");
    logInfo("hello world");
    logInfo("hello world");

    deinitLoggingSystem();

    // FILE *fstream = fopen("/home/bean/bean-raft/log", "a");
    // // write(fstream, )
    // string x("hello world");
    // fwrite(x.c_str(), 1, x.size(), fstream);
    // fflush(fstream);

    return 0;
}
