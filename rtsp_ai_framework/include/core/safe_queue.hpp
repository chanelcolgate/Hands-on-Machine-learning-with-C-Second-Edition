#pragma once

#include <queue>
#include <mutex>
#include <condition_variable>
#include <memory>
#include <chrono>
#include <iostream>
#include <atomic>
#include <optional>

namespace rtsp_ai {

template<typename T>
class SafeQueue {
public:
    struct Metrics {
        std::atomic<uint64_t> total_frames_pushed{0};
        std::atomic<uint64_t> total_frames_dropped{0};
        std::atomic<uint64_t> total_frames_popped{0};
        std::atomic<uint64_t> peak_queue_size{0};
        std::atomic<double> average_wait_time_ms{0.0};
    };

    struct MetricsSnapshot {
        uint64_t total_frames_pushed{0};
        uint64_t total_frames_dropped{0};
        uint64_t total_frames_popped{0};
        uint64_t peak_queue_size{0};
        double average_wait_time_ms{0.0};
    };

    explicit SafeQueue(size_t max_size = 30)
        : max_size_(max_size),
          metrics_(std::make_shared<Metrics>()) {}

    ~SafeQueue() = default;

    // Non-copyable, non-movable
    SafeQueue(const SafeQueue&) = delete;
    SafeQueue operator=(const SafeQueue&) = delete;

    /**
     * Push element to queue. If queue is full, drop the oldest element
     * @param value The element to push
     * @return true if pushed successfully, false if dropped due to overflow
     */
    bool push(T value) {
        std::unique_lock<std::mutex> lock(mutex_);

        bool dropped = false;
        if (queue_.size() >= max_size_) {
            queue_.pop(); // Drop frame cũ để đảm bảo real-time
            metrics_->total_frames_dropped++;
            dropped = true;
        }

        queue_.push(std::move(value));
        metrics_->total_frames_pushed++;

        // Update peak size
        size_t current_size = queue_.size();
        size_t peak = metrics_->peak_queue_size.load();
        while (current_size > peak &&
                !metrics_->peak_queue_size.compare_exchange_weak(peak, current_size)) {
            peak = metrics_->peak_queue_size.load();
        }

        cond_.notify_one();

        if (dropped) {
            log_frame_dropped(current_size);
        }

        return !dropped;
    }

    /**
     * Pop element from queue with timeout
     * @param value Reference to store popped element
     * @param timeout_ms Timeout in milliseconds (0 = wait forever)
     * @return true if element popped successfully, false if timeout
     */
    bool pop(T& value, uint32_t timeout_ms = 0) {
        auto start_time = std::chrono::steady_clock::now();

        std::unique_lock<std::mutex> lock(mutex_);

        bool result;
        if (timeout_ms == 0) {
            // Wait indefinitely
            result = cond_.wait_for(lock, std::chrono::seconds(86400),
                                    [this] { return !queue_.empty(); });
        } else {
            // Wait with timeout
            result = cond_.wait_for(lock, std::chrono::milliseconds(timeout_ms),
                                    [this] { return !queue_.empty(); });
        }
        if (result && !queue_.empty()) {
            value = std::move(queue_.front());
            queue_.pop();
            metrics_->total_frames_popped++;

            // Update average wait time
            auto elapsed = std::chrono::steady_clock::now() - start_time;
            double wait_ms = std::chrono::duration<double, std::milli>(elapsed).count();
            update_average_wait_time(wait_ms);

            return true;
        }

        return false;
    }

    /**
     * Try to pop without blocking
     */
    std::optional<T> try_pop() {
        std::lock_guard<std::mutex> lock(mutex_);

        if (!queue_.empty()) {
            T value = std::move(queue_.front());
            queue_.pop();
            metrics_->total_frames_popped++;
            return value;
        }

        return std::nullopt;
    }

    /**
     * Get current queue size
     */
    size_t size() const {
        std::lock_guard<std::mutex> lock(mutex_);
        return queue_.size();
    }

    /**
     * Check if queue is empty
     */
    bool empty() const {
        std::lock_guard<std::mutex> lock(mutex_);
        return queue_.empty();
    }

    /**
     * Clear all elements from queue
     */
    void clear() {
        std::lock_guard<std::mutex> lock(mutex_);
        while (!queue_.empty()) {
            queue_.pop();
        }
    }

    /**
     * Get metrics
     */
    MetricsSnapshot get_metrics() const {
        return MetricsSnapshot{
            metrics_->total_frames_pushed.load(),
            metrics_->total_frames_dropped.load(),
            metrics_->total_frames_popped.load(),
            metrics_->peak_queue_size.load(),
            metrics_->average_wait_time_ms.load()
        };
    }

    /**
     * Print metrics to stdout
     */
    void print_metrics() const {
        auto m = get_metrics();
        std::cout << "\n=== SafeQueue Metrics ===" << std::endl;
        std::cout << "Total frames pushed: " << m.total_frames_pushed << std::endl;
        std::cout << "Total frames dropped: " << m.total_frames_dropped << std::endl;
        std::cout << "Total frames popped: " << m.total_frames_popped << std::endl;
        std::cout << "Peak queue size: " << m.peak_queue_size << std::endl;
        std::cout << "Average wait time: " << m.average_wait_time_ms << " ms" << std::endl;

        if (m.total_frames_pushed > 0) {
            double drop_rate = (static_cast<double>(m.total_frames_dropped) /
                                m.total_frames_pushed) * 100.0;
            std::cout << "Drop rate: " << drop_rate << "%" << std::endl;
        }
        std::cout << "========================\n" << std::endl;
    }

private:
    std::queue<T> queue_;
    mutable std::mutex mutex_;
    std::condition_variable cond_;
    size_t max_size_{30};
    std::shared_ptr<Metrics> metrics_;

    void log_frame_dropped(size_t current_size) {
        if (metrics_->total_frames_dropped % 30 == 0) { // Log every 30th drop
            std::cerr << "[SafeQueue] Frame dropped! Queue size: " << current_size
                    << ", Total dropped: " << metrics_->total_frames_dropped << std::endl;
        }
    }

    void update_average_wait_time(double wait_ms) {
        // Simple exponential moving average
        double current_avg = metrics_->average_wait_time_ms.load();
        double alpha = 0.1; // Smoothing factor
        double new_avg = (alpha * wait_ms) + ((1.0 - alpha) * current_avg);
        metrics_->average_wait_time_ms.store(new_avg);
    }


};
} // namespace rtsp_ai
