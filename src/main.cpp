
#include <iostream>
#include <vector>

#include "slice.hpp"

using namespace std;

int main() {
    /* config文件是如何确定的，同时最好抽象一个state machine, kv db. */

    std::cout << "hello world" << std::endl;
    Slice s("hello slice");
    s.appendFmt("bbb: %s", "xxx");
    std::cout << s << endl;
    return 0;
}
