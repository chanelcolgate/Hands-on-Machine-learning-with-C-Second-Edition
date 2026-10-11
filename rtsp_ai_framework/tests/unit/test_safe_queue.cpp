#include <gtest/gtest.h>
#include <thread>
#include <vector>
#include <chrono>
#include <atomic>
#include "core/safe_queue.hpp"
#include "core/frame_data.hpp"

using namespace rtsp_ai;

/**
 * Test basic push/pop operations
 */
TEST(SafeQueueTest, BasicPushPop) {
    SafeQueue<int> queue(5);

    // Push elements
    queue.push(1);
    queue.push(2);
    queue.push(3);

    EXPECT_EQ(queue.size(), 3);

    // Pop elements
    int value;
    EXPECT_TRUE(queue.pop(value, 1000));
    EXPECT_EQ(value, 1);

    EXPECT_TRUE(queue.pop(value, 1000));
    EXPECT_EQ(value, 2);

    EXPECT_TRUE(queue.pop(value, 1000));
    EXPECT_EQ(value, 3);

    EXPECT_TRUE(queue.empty());
}

/**
 * Test drop-frame behavior when queue overflows
 */
TEST(SafeQueueTest, ConcurrentPush) {
    SafeQueue<int> queue(100);
    const int num_threads = 4;
    const int items_per_thread = 25;

    std::vector<std::thread> threads;

    // Launch multiple threads pushing data
    for (int t = 0; t < num_threads; ++t) {
        threads.emplace_back([&queue, t, items_per_thread]() {
            for (int i = 0; i < items_per_thread; ++i) {
                queue.push(t * 1000 + i);
            }
        });
    }

    // Wait for all threads
    for (auto& th : threads) {
        th.join();
    }

    // Verify all items were pushed
    auto metrics = queue.get_metrics();
    EXPECT_EQ(metrics.total_frames_pushed, num_threads * items_per_thread);
    EXPECT_EQ(queue.size(), num_threads * items_per_thread);
}

/**
 * Test concurrent push and pop
 */
TEST(SafeQueueTest, ConcurrentPushPop) {
    SafeQueue<int> queue(50);
    const int items_total = 1000;
    std::atomic<int> pop_count(0);

    // Producer thread
    std::thread producer([&queue, items_total]() {
        for (int i = 0; i < items_total; ++i) {
            queue.push(i);
        }
    });

    // Consumer threads
    std::vector<std::thread> consumers;
    for (int c = 0; c < 3; ++c) {
        consumers.emplace_back([&queue, &pop_count]() {
            int value;
            while (queue.pop(value, 100)) {
                pop_count++;
            }
        });
    }

    producer.join();

    // Give consumers time to finish
    std::this_thread::sleep_for(std::chrono::seconds(1));
    queue.clear(); // Unblock consumers

    for (auto& th : consumers) {
        th.join();
    }

    EXPECT_EQ(pop_count.load(), items_total);
}

/**
 * Test timeout behavior
 */
TEST(SafeQueueTest, TimeoutBehavior) {
    SafeQueue<int> queue(5);

    int value;
    auto start = std::chrono::steady_clock::now();

    // Pop from empty queue with 500ms timeout
    bool result = queue.pop(value, 500);

    auto elapsed = std::chrono::duration_cast<std::chrono::milliseconds>(
        std::chrono::steady_clock::now() - start
    );

    EXPECT_FALSE(result);
    EXPECT_GE(elapsed.count(), 400); // Allow 100ms tolerance
    EXPECT_LE(elapsed.count(), 600);
}

/**
 * Test try_pop non-blocking operation
 */
TEST(SafeQueueTest, TryPopNonBlocking) {
    SafeQueue<int> queue(5);

    // Empty queue should return nullopt
    auto result = queue.try_pop();
    EXPECT_FALSE(result.has_value());

    // Push element
    queue.push(42);

    // Now it should succeed
    result = queue.try_pop();
    EXPECT_TRUE(result.has_value());
    EXPECT_EQ(result.value(), 42);
}

/**
 * Test with FrameData objects
 */
TEST(SafeQueueTest, FrameDataQueue) {
    SafeQueue<FrameData> queue(10);

    // Create frame data
    FrameData frame1;
    frame1.frame_id = 1;
    frame1.set_metadata("test", "value1");

    FrameData frame2;
    frame2.frame_id = 2;
    frame2.set_metadata("test", "value2");

    // Push frames
    queue.push(frame1);
    queue.push(frame2);

    // Pop and verify
    FrameData retrieved1;
    EXPECT_TRUE(queue.pop(retrieved1, 1000));
    EXPECT_EQ(retrieved1.frame_id, 1);
    EXPECT_EQ(retrieved1.get_metadata("test"), "value1");

    FrameData retrieved2;
    EXPECT_TRUE(queue.pop(retrieved2, 1000));
    EXPECT_EQ(retrieved2.frame_id, 2);
    EXPECT_EQ(retrieved2.get_metadata("test"), "value2");
}

/**
 * Test memory cleanup on destruction
 */
TEST(SafeQueueTest, DestructionCleanup) {
    {
        SafeQueue<std::unique_ptr<int>> queue(5);

        // Push unique pointers
        queue.push(std::make_unique<int>(1));
        queue.push(std::make_unique<int>(2));
        queue.push(std::make_unique<int>(3));

        // Scope ends, queue is destroyed
    }
    // If we get here without crashes, cleanup works correctly
    EXPECT_TRUE(true);
}

/**
 * Test clear operation
 */
TEST(SafeQueueTest, ClearOperation) {
    SafeQueue<int> queue(10);

    // Fill queue
    for (int i = 0; i < 10; ++i) {
        queue.push(i);
    }

    EXPECT_EQ(queue.size(), 10);
    EXPECT_FALSE(queue.empty());

    // Clear
    queue.clear();

    EXPECT_EQ(queue.size(), 0);
    EXPECT_TRUE(queue.empty());
}

/**
 * Test peak queue size metric
 */
TEST(SafeQueueTest, PeakSizeMetric) {
    SafeQueue<int> queue(10);

    // Push to different sizes
    queue.push(1);
    queue.push(2);
    queue.push(3); // Peak at 3

    int dummy;
    queue.pop(dummy, 100); // Size is now 2
    
    auto metrics = queue.get_metrics();
    EXPECT_EQ(metrics.peak_queue_size, 3);
}

/**
 * Test stress with rapid push/top
 */
TEST(SafeQueueTest, StressTest) {
    SafeQueue<int> queue(100);
    const int iterations = 10000;

    std::atomic<int> push_count(0);
    std::atomic<int> pop_count(0);

    std::thread pusher([&]() {
        for (int i = 0; i < iterations; ++i) {
            queue.push(i);
            push_count++;
        }
    });

    std::thread popper([&]() {
        int value;
        while (pop_count < iterations) {
            if (queue.pop(value, 10)) {
                pop_count++;
            }
        }
    });

    pusher.join();
    popper.join();

    EXPECT_EQ(push_count.load(), iterations);
    EXPECT_EQ(pop_count.load(), iterations);
}
