#include <gtest/gtest.h>

#include "log_builder.h"

TEST(LogBuilderTest, BasicLogMessage) {
    LogBuilder builder;
    builder.addLog("INFO", "Test log message");
    std::string result = builder.getLogs();
    EXPECT_EQ(result, "[INFO] Test log message");
}

int main(int argc, char **argv) {
    ::testing::InitGoogleTest(&argc, argv);
    return RUN_ALL_TESTS();
}
