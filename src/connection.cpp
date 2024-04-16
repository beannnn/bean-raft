

// 挂载到eventLoop上
// 
// class rPipe {
// };

// class 
// 注册不同类型的信息. 
// type
// data-type.
// read process function.
// single 


/* Connection需要绑定一个read, write function. */
class Connection {
public:
    Connection();
    ~Connection();
    

    // 实现是强依赖eventloop的吧, 最难的部分。
    // 首先需要明确，

    // 连接有很多数据应该怎么处理的？
    // 学习一下readClusterReply的处理方法？
    
    // 本质就是对fd的包装。
    // 

    int fd;

};
