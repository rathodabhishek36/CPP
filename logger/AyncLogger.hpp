#pragma once

#include <iostream>
#include <fstream>
#include <thread>
#include <condition_variable>

#include "MultiProducerRingBuffer.hpp"

class AsyncLogger {
    std::ofstream logFile_;
    MultiProducerSingleConsumerRingBuffer<std::string> buffer_;
    std::thread loggerThread_;
    std::chrono::milliseconds flushInterval_;
    std::size_t maxFlushSize_;
    std::condition_variable startFlushCv_;
    std::mutex startFlush_;
    bool loggingFinished_ = false;

    void work() {
        while (!loggingFinished_ || !buffer_.empty()) {
            std::unique_lock<std::mutex> guard (startFlush_);
            auto timeout = std::chrono::steady_clock::now() + flushInterval_;
            startFlushCv_.wait_until(guard, timeout, [&] () { return buffer_.size() >= maxFlushSize_; });
            guard.unlock();

            auto batchSize = std::min(buffer_.size(), maxFlushSize_);
            while (batchSize--) {
                auto message = buffer_.pop();
                if (message.has_value()) {
                    logFile_ << message.value() << std::endl;
                }
            }
            logFile_.flush();
        }
    }

public:
    AsyncLogger(const std::string& logFile
              , const std::size_t bufferSize = 10'000
              , const std::chrono::milliseconds flushInterval = std::chrono::milliseconds(100)
              , const std::size_t maxFlushSize = 1000)
              : logFile_(logFile)
              , buffer_(bufferSize)
              , loggerThread_(&AsyncLogger::work, this) 
              , flushInterval_(flushInterval)
              , maxFlushSize_(maxFlushSize)
    {
    }

    ~AsyncLogger() {
        loggingFinished_ = true;
        startFlushCv_.notify_one();
        if (buffer_.size() > 0) {
            std::this_thread::sleep_for(std::chrono::milliseconds(1000));
        }
        if (loggerThread_.joinable()) {
            loggerThread_.join();
        }
        logFile_.close();
    }

    // Non-Blocking
    bool try_log(const std::string& message) {
        if (!buffer_.try_push(message)) [[unlikely]] {
            std::cerr << "Buffer is full, dropping message: " << message << std::endl;
            return false;
        }
        startFlushCv_.notify_one();
        return true;
    }

    // Blocking
    void log(const std::string& message) {
        while (!buffer_.try_push(message)) {
            std::this_thread::yield();
        }
        startFlushCv_.notify_one();
    }

};