#include <gtest/gtest.h>

#include "log_builder.h"
#include "log_collector.h"

TEST(LogBuilderTest, BasicLogMessage) {
    xubinh_server::LogCollector::set_base_name("hello_world");

    LOG_INFO << "hello, world!";
}

int main(int argc, char **argv) {
    ::testing::InitGoogleTest(&argc, argv);

    return RUN_ALL_TESTS();
}
