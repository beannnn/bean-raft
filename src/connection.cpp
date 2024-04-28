
#include <functional>
#include <vector>
#include <map>
#include <deque>
#include <unordered_map>
#include <string>

#include <unistd.h>
#include <string.h>

#include "eventloop.hpp"
#include "slice.hpp"
#include "anet.hpp"

enum connectionStatus {
    not_connectted,
    connectting,
    connectted,
    error
};

enum raftMembership {
    leader,
    candidiate,
    follower
};

enum messageType {
    requestVote,
    appendEntries
};

/* 存放解析协议后的请求 */
class request {
public:
    request() : data(), written(0), offsets(), lengths(), parsed(false) {
        
    }
    ~request() = default;
    

    Slice data;
    int written;
    std::vector<int> offsets;
    std::vector<int> lengths;
    bool parsed;
};

class reply {
public:
    
};

// 1，如何解析成这样
// leader id?
// ip或者ipv4->uint64
// 
struct VoteRequest {
    bool decodeFromRequest(request *req) {
        if (req->offsets.size() != 4) {
            return false;
        }
        
    }
    uint64_t term;
    uint64_t candidateId;
    uint64_t lastLogIndex;
    uint64_t lastLogTerm;
};

// 2，decode
struct VoteReply {
    bool encodeToSlice(Slice *s) {
        // s.append("*2\r\n");
        // s.append(term);
        // s.append(voteGranted);
    }
    uint64_t term;
    bool voteGranted;
};

struct AppendEntriedRequest {
    uint64_t term;
    uint64_t leaderId;
    uint64_t prevLogIndex;
    uint64_t prevLogTerm;
    // entries[] vector<int> nums;
    uint64_t leaderCommit;
};

/* 还需要分配 */
struct AppendEntriesReply {
    bool encodeToSlice(Slice *s) {
        s->append("*2\r\n");
        // s->append(term);
        // s->append(voteGranted);
    }
    uint64_t term;
    bool success;
};


// 这个是主动建立的连接
// 被动建立的连接呢
// 
class Connection {
public:
    Connection(char *ip, int port);
    ~Connection() {
        
    }
    /* 对于被动连接的情况，使用这个函数 */
    void initWithFileDescriptor() {
        
    }
    /* TODO: need to implementation */
    bool blockConnect(long long milliseconds);
    void nonblockConnect(char *ip, int port) {
        status = connectting;
        /* triggle a function. */
        char errbuf[ANET_ERR_LEN]; 
        fd = anetTcpNonBlockConnect(errbuf, ip, port);
        if (fd > 0) {
            status = connectting;
        }
    }

    bool readFromSocket() {
        /* check the status */ 
        if (status == connectting) {
            status = connectted;
        }
        char buf[4096];
        int nread = read(fd, buf, 4096);
        if (nread < 0) {
            if (errno == EAGAIN || errno == EWOULDBLOCK || errno == EINTR) {
                return;
            }
        } else if (nread == 0) {
            status = not_connectted;
            errmsg = Slice("peer close the connection");
            return;
        }
        data.append(buf, nread);
    }
    // 需要减少拷贝，那么就需要和
    // 那么外部来进行注册。
    // 内部如何知道是否写完了？
    int writeToSocket(const char *data, int len) {
        int nwrite = 0;
        nwrite = write(fd, data, len);
        if (nwrite < 0) {
            if (errno == EAGAIN || errno == EWOULDBLOCK || errno == EINTR) {
                return -1;
            }
            errmsg = strerror(errno);
            fd = -1;
            status = not_connectted;
            return -1;
        } else if (nwrite == 0) {
            fd = -1;
            status = not_connectted;
            return 0;
        }
        return nwrite;
    }
    int fd;
    connectionStatus status;
    Slice data;
    Slice errmsg;
};

    // 有一个问题，如果我不知道这些connection的信息
    // 我怎么去写数据？如果推入一些数据，
    // 队列 + Slice？

    /* 现在的问题就是怎么去快速的进行读取？另一个问题就是资源的释放问题，我们是否需要一个单独的 */
    /* 实现一种特殊的释放器来适配流，我们会自动切分前面的空间。zcalloc的时候会尝试去查看后续的空间，如果没有，那么就是 */
    // 存在一个队列来进行封装。
    // 每次解析得到了一个完整的请求怎么做？
    // 支持pipeline的
    /* stream allocator */
    // RTC模型，单个线程来进行落盘操作。
    // 但是这种不一致如何处理？
    // 
    // 我们需要合并WAL + levelDB's WAL.
    // 收到请求的才可以释放掉。

    // 非常简单的single-thread的

    // 但是合并以及compaction怎么办？
    // 这个就是最大的问题，
    // 首先我们可以使用raft的log作为我们的WAL

    // 我们的数据不需要额外的落盘
    // 

/* raftNode */
// 首先是connection进行发送
// write就是发送一个一个的request，其实
// based on configuration file
// 这个节点的role需要知道
class raftNode {
    /* 读取函数 */
    void readHandler() {
        bool ret = connection.readFromSocket();
        if (ret == false) {
            printf("fd error");
            
        }

        /* parse RESP data, 解析为一个一个request */
        

    }


    // 写入fd成功实际上就ok了，但是我如何保证
    // pipeline的时候如果发生了成员变更，肯定是需要失效很多东西的, 不要担心.
    // ip->id
    void writeHandler() {
        while (!replys.empty()) {
            Slice &data = replys.front();
            
            int should_write = data.size() - reply_written;
            int ret = connection.writeToSocket(static_cast<char *>(data) + reply_written, should_write);
            /* 写入失败 */
            if (ret == -1) {
                // connection
                break;
            }
            reply_written += ret;
            if (reply_written < data.size()) {
                /* this connection is busy. */
                break;
            }
            replys.pop_front();
            reply_written = 0;
        }

        if (replys.empty()) {
            /* remove write handler */
        }
    }
    
    /*  */
    
    int parsed_offset;
    std::deque<request *> requests;
    std::deque<Slice> replys;
    int reply_written;
    Connection connection;
    EventLoop *el;

    /* raft member, 我需要知道其他节点包含哪些信息 */
    raftMembership role;
    int term;
    int votefor;
};

/* 根据请求来进行状态转移。 */
// raft之下挂了一个eventloop.
// 开始之后从各个节点这里发送信息
// 更加快速的failover, 持续keepalive
// https://blog.csdn.net/weixin_51322383/article/details/132144069
class raft {
public:
    // 处理某一个raft节点发送来的信息
    void processRequestVote(VoteRequest *req, VoteReply *reply) {
        if (req->term < term) {
            reply->voteGranted = false;
            reply->term = term;
            return;
        }
        
        if (req->lastLogIndex) {
            
        }
        
        /* Grant for this vote request. */
        
        reply->voteGranted = true;
    }
    void processAppendEntries(AppendEntriedRequest *req, AppendEntriesReply *reply) {
        
    }
    /* 100ms time loop. */
    int raftCron() {
        switch (role) {
        case leader:
            // for (auto ite = peers.begin(); ite != peers.end(); ++ite) {
                
            // }

            break;
        case candidiate:
            

            break;
        case follower:
            

            /* TODO: Optimize: hold basic ping for othrears follower, for faster failover. */
            // for (auto ite = peers.begin(); ite != peers.end(); ++ite) {
            //     raftNode *node = ite->second;
                
            // }
            break;
        }

        
        return 100;
    }

    /* Persistent state */
    uint64_t currentTerm;
    uint64_t voteFor;
    std::deque<Slice> log;

    /* commitIndex */
    uint64_t commmitIndex;
    uint64_t lastApplied;
    
    /* 从这个角度来说，我们不需要一个多线程版本的levelDB, 单线程足够好。但是有一个致命的问题，我们的持久化操作不可以阻塞循环
     * 这个如何实现. 存储系统的构建需要sync的即可, log如何实现？ */
    std::unordered_map<uint64_t, uint64_t> nextIndex;
    std::unordered_map<uint64_t, uint64_t> matchIndex;
    std::unordered_map<uint64_t, raftNode *> peers;

    raftMembership role;
    uint64_t term;
    uint64_t votefor;
    
    // commit
    uint64_t commit_index;

    // leader state
    std::unordered_map<std::string, uint64_t> next_index;
    std::unordered_map<std::string, uint64_t> match_index;

    /* TODO: 断线重连接，所以可以按照block进行发送 */
    std::unordered_map<std::string, std::string> stateMachine;
};
