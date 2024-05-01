
#include <string>
#include "sds.hpp"

/* 所有的C++定义的都是浅拷贝 */
class Slice {
public:
    Slice();
    Slice(const char *);
    Slice(const char *s, int len);
    Slice(const Slice &);
    Slice(Slice &&);
    Slice &operator=(const Slice &);
    Slice &operator=(Slice &&);
    bool operator==(const Slice &);
    ~Slice();

    /* 能够直接进行转型 */
    operator const char *() const;
    operator char *();

    size_t size() const;
    size_t capacity() const;
    void reserve(size_t);
    void shrinkToFit();
    void clear();

    Slice dup() const;
    void appendFmt(const char *fmt, ...);
    void appendPrintf(const char *fmt, ...);
    void append(const Slice &);
    void append(const std::string &);
    void append(const char);
    void append(const char *);
    void append(const char *, int len);
    void append(long long);
    
    /* resp handler */
    void appendSimpleString();
    void appendSimpleNumber();
    void appendError();
    void generateArrayHeader();
    void appendBulkString();
    void appendInteger();
    void appendNull();
    
private:
    void _initIfNeed();

    sds data;
};
