#include "gtest/gtest.h"
#include "gmock/gmock.h"

#include <iostream>

TEST(compileTest, gmockTest1) {
    std::cout << "compile correct!" << std::endl;
    ASSERT_EQ(NULL, NULL);
}