#include "gtest/gtest.h"
#include "gmock/gmock.h"

#include <unistd.h>
#include <stdlib.h>
#include <climits>
#include <vector>

#include "sds.hpp"

class SdsTest : public testing::TestWithParam<int> {
protected:
    virtual void SetUp() {
    }
    virtual void TearDown() {
    }
};

TEST_F(SdsTest, sdsFormat) {
    sds x = sdsnew("foo"), y;
    ASSERT_TRUE(sdslen(x) == 3 && memcmp(x,"foo\0",4) == 0);

    sdsfree(x);
    x = sdsnewlen("foo",2);
    ASSERT_TRUE(sdslen(x) == 2 && memcmp(x,"fo\0",3) == 0);

    x = sdscat(x,"bar");
    ASSERT_TRUE(sdslen(x) == 5 && memcmp(x,"fobar\0",6) == 0);

    x = sdscpy(x,"a");
    ASSERT_TRUE(sdslen(x) == 1 && memcmp(x,"a\0",2) == 0);

    x = sdscpy(x,"xyzxxxxxxxxxxyyyyyyyyyykkkkkkkkkk");
    ASSERT_TRUE(sdslen(x) == 33 && memcmp(x,"xyzxxxxxxxxxxyyyyyyyyyykkkkkkkkkk\0",33) == 0);

    sdsfree(x);
    x = sdscatprintf(sdsempty(),"%d",123);
    ASSERT_TRUE(sdslen(x) == 3 && memcmp(x,"123\0",4) == 0);

    sdsfree(x);
    x = sdsnew("--");
    x = sdscatfmt(x, "Hello %s World %I,%I--", "Hi!", LLONG_MIN,LLONG_MAX);
    ASSERT_TRUE(sdslen(x) == 60 &&
                memcmp(x,"--Hello Hi! World -9223372036854775808,"
                "9223372036854775807--",60) == 0);

    sdsfree(x);
    x = sdsnew("--");
    x = sdscatfmt(x, "%u,%U--", UINT_MAX, ULLONG_MAX);
    ASSERT_TRUE(sdslen(x) == 35 &&
                memcmp(x,"--4294967295,18446744073709551615--",35) == 0);

    sdsfree(x);
    x = sdsnew(" x ");
    sdstrim(x," x");
    ASSERT_EQ(sdslen(x), 0);

    sdsfree(x);
    x = sdsnew(" x ");
    sdstrim(x," ");
    ASSERT_TRUE(sdslen(x) == 1 && x[0] == 'x');

    sdsfree(x);
    x = sdsnew("xxciaoyyy");
    sdstrim(x,"xy");
    ASSERT_TRUE(sdslen(x) == 4 && memcmp(x,"ciao\0",5) == 0);

    y = sdsdup(x);
    sdsrange(y,1,1);
    ASSERT_TRUE(sdslen(y) == 1 && memcmp(y,"i\0",2) == 0);

    sdsfree(y);
    y = sdsdup(x);
    sdsrange(y,1,-1);
    ASSERT_TRUE(sdslen(y) == 3 && memcmp(y,"iao\0",4) == 0);

    sdsfree(y);
    y = sdsdup(x);
    sdsrange(y,-2,-1);
    ASSERT_TRUE(sdslen(y) == 2 && memcmp(y,"ao\0",3) == 0);

    sdsfree(y);
    y = sdsdup(x);
    sdsrange(y,2,1);
    ASSERT_TRUE(sdslen(y) == 0 && memcmp(y,"\0",1) == 0);

    sdsfree(y);
    y = sdsdup(x);
    sdsrange(y,1,100);
    ASSERT_TRUE(sdslen(y) == 3 && memcmp(y,"iao\0",4) == 0);

    sdsfree(y);
    y = sdsdup(x);
    sdsrange(y,100,100);
    ASSERT_TRUE(sdslen(y) == 0 && memcmp(y,"\0",1) == 0);

    sdsfree(y);
    sdsfree(x);
    x = sdsnew("foo");
    y = sdsnew("foa");
    ASSERT_GT(sdscmp(x,y), 0);

    sdsfree(y);
    sdsfree(x);
    x = sdsnew("bar");
    y = sdsnew("bar");
    ASSERT_EQ(sdscmp(x,y), 0);

    sdsfree(y);
    sdsfree(x);
    x = sdsnew("aar");
    y = sdsnew("bar");
    ASSERT_LT(sdscmp(x,y), 0);

    sdsfree(y);
    sdsfree(x);
    x = sdsnewlen("\a\n\0foo\r",7);
    y = sdscatrepr(sdsempty(),x,sdslen(x));
    ASSERT_EQ(memcmp(y,"\"\\a\\n\\x00foo\\r\"",15), 0);

    unsigned int oldfree;
    char *p;
    int step = 10, j, i;

    sdsfree(x);
    sdsfree(y);
    x = sdsnew("0");
    ASSERT_TRUE(sdslen(x) == 1 && sdsavail(x) == 0);

    /* Run the test a few times in order to hit the first two
     * SDS header types. */
    for (i = 0; i < 10; i++) {
        int oldlen = sdslen(x);
        x = sdsMakeRoomFor(x,step);
        int type = x[-1]&SDS_TYPE_MASK;

        ASSERT_TRUE(sdslen(x) == oldlen);
        if (type != SDS_TYPE_5) {
            ASSERT_TRUE(sdsavail(x) >= step);
            oldfree = sdsavail(x);
        }
        p = x+oldlen;
        for (j = 0; j < step; j++) {
            p[j] = 'A'+j;
        }
        sdsIncrLen(x,step);
    }
    ASSERT_EQ(memcmp("0ABCDEFGHIJABCDEFGHIJABCDEFGHIJABCDEFGHIJABCDEFGHIJABCDEFGHIJABCDEFGHIJABCDEFGHIJABCDEFGHIJABCDEFGHIJ",x,101), 0);
    ASSERT_EQ(sdslen(x), 101);
    sdsfree(x);
}
