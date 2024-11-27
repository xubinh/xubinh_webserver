#include <gtest/gtest.h>

#include "log_builder.h"

TEST(LogBuilderTest, BasicLogMessage) {
    LOG_INFO << "hello, world!";
}

int main(int argc, char **argv) {
    ::testing::InitGoogleTest(&argc, argv);
    return RUN_ALL_TESTS();
}
