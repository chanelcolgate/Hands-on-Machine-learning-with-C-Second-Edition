#pragma once

#include <thread>
#include <atomic>
#include <iostream>
#include <string>
#include <functional>
#include <chrono>

namespace rtsp_ai {

/**
 * Base class for all components in the pipeline
 * Implements UVM-like lifecycle management with state machine
 */

class BaseComponent {
public:
    enum class ComponentState {
        IDLE,       // Initial state
        STARTING,   // Transitioning to RUNNING
        RUNNING,    // Component is actively processing
        STOPPING,   // Transitioning to STOPPED
        STOPPED,    // Component has stopped
        ERROR       // Error state
    };

    virtual ~BaseComponent() {
        stop();
        if (worker_thread_.joinable()) {
            worker_thread_.join();
        }
    };

    // Non-copyable, non-movable
    BaseComponent(const BaseComponent&) = delete;
    BaseComponent& operator=(const BaseComponent&) = delete;

    /*
     * Start the component
     * Initializes resources and starts the worker thread
     */
    virtual void start() {
        if (state_.load() != ComponentState::IDLE) {
            std::cerr << "[" << get_name() << "] Cannot start: already in state "
                      << static_cast<int>(state_.load()) << std::endl;
            return;
        }

        try {
            state_.store(ComponentState::STARTING);

            // Call derived class initialization
            initialize();

            // Start worker thread
            worker_thread_ = std::thread([this] { this->run(); });

            state_.store(ComponentState::RUNNING);
            std::cout << "[" << get_name() << "] Started successfully" << std::endl;
        } catch (const std::exception& e) {
            state_.store(ComponentState::ERROR);
            std::cerr << "[" << get_name() << "] Failed to start: " << e.what() << std::endl;
        }
    }

    /**
     * Stop the component gracefully
     * Signals the worker thread to stop and waits for it
     */
    virtual void stop() {
        ComponentState current_state= state_.load();

        if (current_state == ComponentState::IDLE ||
            current_state == ComponentState::STOPPED ||
            current_state == ComponentState::ERROR) {
            return;
        }

        state_.store(ComponentState::STOPPING);
        running_.store(false);

        // Give thread time to finish naturally
        if (worker_thread_.joinable()) {
            if (worker_thread_.get_id() != std::this_thread::get_id()) {
                // Wait up to 5 seconds for thread to finish
                auto deadline = std::chrono::steady_clock::now() + std::chrono::seconds(5);

                while (worker_thread_.joinable() &&
                        std::chrono::steady_clock::now() < deadline) {
                    std::this_thread::sleep_for(std::chrono::milliseconds(100));
                }

                if (worker_thread_.joinable()) {
                    std::cerr << "[" << get_name() << "] Worker thread did not terminate in time"
                              << std::endl;
                }
            }
        }

        try {
            finalize();
        } catch (const std::exception& e) {
            std::cerr << "[" << get_name() << "] Error during finalization: "
                      << e.what() << std::endl;
        }

        state_.store(ComponentState::STOPPED);
        std::cout << "[" << get_name() << "] Stopped" << std::endl;
    };

    /**
     * Get current state of the component
     */
    ComponentState get_state() const {
        return state_.load();
    }

    /**
     * Check if component is running
     */
    bool is_running() const {
        return running_.load() && state_.load() == ComponentState::RUNNING;
    }

    /**
     * Get component name
     */
    virtual std::string get_name() const {
        return "BaseComponent";
    }

protected:
    BaseComponent()
        : running_(false),
        state_(ComponentState::IDLE) {}

    /**
     * Main processing loop - override in derived classes
     */
    virtual void run() {
        running_.store(true);

        while (running_.load()) {
            std::this_thread::sleep_for(std::chrono::milliseconds(10));
        }
    }

    /**
     * Initialization hook - called before thread starts
     */
    virtual void initialize() {
        // Override in derived classes
    }

    /*
     * Finalization hook - called before thread joins
     */
    virtual void finalize() {
        // Override in derived classes
    }

    /**
     * Helper to check if we should continue running
     */
    bool should_run() const {
        return running_.load();
    }

    /**
     * Set running flag
     */
    void set_running(bool running) {
        running_.store(running);
    }

private:
    std::thread worker_thread_;
    std::atomic<bool> running_{false};
    std::atomic<ComponentState> state_{ComponentState::IDLE};
};

} // namespace rtsp_ai
