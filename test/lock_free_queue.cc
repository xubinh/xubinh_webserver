#include <gtest/gtest.h>
#include <stdexcept>

#include "util/lock_free_queue.h"

TEST(LockFreeQueueTest, PushAndPop) {
    xubinh_server::util::SpscLockFreeQueue<int> q;

    q.push(1);
    q.push(2);
    q.push(3);
    if (*q.pop() != 1) {
        throw std::runtime_error("error");
    }
    if (*q.pop() != 2) {
        throw std::runtime_error("error");
    }
    if (*q.pop() != 3) {
        throw std::runtime_error("error");
    }
}

int main(int argc, char **argv) {
    testing::InitGoogleTest(&argc, argv);

    // Disable output capturing
    testing::FLAGS_gtest_catch_exceptions = false;

    return RUN_ALL_TESTS();
}
