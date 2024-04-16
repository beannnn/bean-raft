
#include "slice.hpp"
#include "sds.hpp"
#include "util.hpp"

Slice::Slice(): data(NULL) {
}

Slice::Slice(const char *s) {
    data = sdsnew(s);
}

Slice::Slice(const char *s, int len) {
    data = sdsnewlen(s, len);
}

Slice::Slice(const Slice &s) {
    if (data) sdsfree(data);
    data = sdsnewlen(reinterpret_cast<const char *>(s.data), sdslen(s.data));
}

Slice::Slice(Slice &&s) {
    if (data) sdsfree(data);
    data = s.data;
    s.data = NULL;
}

Slice &Slice::operator=(const Slice &s) {
    if (data) sdsfree(data);
    data = sdsnewlen(reinterpret_cast<const char *>(s.data), sdslen(s.data));
}

Slice &Slice::operator=(Slice &&s) {
    if (data) sdsfree(data);
    data = s.data;
    s.data = NULL;
}

bool Slice::operator==(const Slice &s) {
    if (!data || !s.data) return false;
    return sdscmp(data, s.data) == 0;
}

Slice::~Slice() {
    if (data) sdsfree(data);
}

Slice::operator const char *() const {
    return reinterpret_cast<const char *>(data);
}

Slice::operator char *() {
    return reinterpret_cast<char *>(data);
}

inline void Slice::_initIfNeed() {
    if (!data) data = sdsempty();
}

void Slice::clear() {
    if (data) sdsfree(data);
    data = NULL;
}

size_t Slice::size() const {
    return data == NULL ? 0 : sdslen(data);
}

size_t Slice::capacity() const {
    return data == NULL ? 0 : sdslen(data) + sdsavail(data);
}

void Slice::reserve(size_t new_size) {
    _initIfNeed();

    sdsMakeRoomFor(data, new_size);
}

void Slice::shrinkToFit() {
    if (!data) return;
    
    sdsRemoveFreeSpace(data);
}

Slice Slice::dup() const {
    return Slice(data, sdslen(data));
}

/* wrapper for sdscatfmt
 * %s - C String
 * %S - SDS string
 * %i - signed int
 * %I - 64 bit signed integer (long long, int64_t)
 * %u - unsigned int
 * %U - 64 bit unsigned integer (unsigned long long, uint64_t)
 * %% - Verbatim "%" character.
 * */
void Slice::appendFmt(char const *fmt, ...) {
    _initIfNeed();

    va_list ap;
    va_start(ap, fmt);
    data = sdscatvfmt(data, fmt, ap);
    va_end(ap);
}

void Slice::appendPrintf(const char *fmt, ...) {
    _initIfNeed();

    va_list ap;
    va_start(ap, fmt);
    data = sdscatvprintf(data, fmt, ap);
    va_end(ap);
}

void Slice::append(const Slice &s) {
    if (!s.data) return;
    _initIfNeed();
    
    data = sdscatlen(data, s.data, sdslen(s.data));
}

void Slice::append(const std::string &s) {
    _initIfNeed();

    data = sdscatlen(data, s.c_str(), s.size());
}

void Slice::append(const char *s) {
    _initIfNeed();
    
    data = sdscat(data, s);
}

void Slice::append(const char *s, int len) {
    _initIfNeed();
    
    data = sdscatlen(data, s, len);
}

void Slice::append(const char s) {
    _initIfNeed();

    sdscatlen(data, &s, 1);
}

void Slice::append(long long number) {
    _initIfNeed();

    char buffer[64];
    int num_len = ll2string(buffer, 64, number);
    sdscatlen(data, buffer, num_len);
}
