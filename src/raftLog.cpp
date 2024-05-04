
#include "slice.hpp"

/* how to async  */

/* 每次写入4KB block */
/* 可以通过一些命令来获取当前raftlog的状态 */

/* 需要实现truncate函数来执行 */

/* commit之后才可以apply才可以提供一个完整的数据。 */

/* 有一些问题：1，日志只会写在一个盘 */
/* Logger持久化引擎 + KV state machine引擎融合，或者说耦合。 */

/* 1，合并日志 */
/* 2, 单线程读写的前提下的并发模型 */
/* 3,  */

/* raft只是log */

// 多个盘存储数据的结构？ 如何实现？
// 多核系统，cpu如何利用多核的处理能力。

class RaftLog {
public:

private:

    
};
