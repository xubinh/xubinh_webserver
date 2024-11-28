#include <gtest/gtest.h>

#include "log_builder.h"
#include "log_collector.h"

TEST(LogBuilderTest, BasicLogMessage) {
    xubinh_server::LogCollector::set_base_name("hello_world");

    LOG_INFO << "hello, world!";
}

TEST(LogBuilderTest, BasicLogMessage2) {
    xubinh_server::LogCollector::set_base_name("hello_world");

    int i = 65535;

    double d = 3.14;

    char *a = new char[32]{"123456"};

    int *p = new int;

    LOG_INFO << i;
    LOG_INFO << d;
    LOG_INFO << a;
    LOG_INFO << p;

    delete p;

    delete a;
}

int main(int argc, char **argv) {
    ::testing::InitGoogleTest(&argc, argv);

    // Disable output capturing
    testing::FLAGS_gtest_catch_exceptions = false;

    return RUN_ALL_TESTS();
}
