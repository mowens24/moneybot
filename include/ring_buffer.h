#ifndef RING_BUFFER_H
#define RING_BUFFER_H

#include <array>
#include <atomic> // For thread-safe operations
#include <string>

// Lock-free ring buffer for high-speed data reception
class RingBuffer {
public:
    // Constructor: Initialize buffer with given size
    explicit RingBuffer(size_t size) : buffer_(size), head_(0), tail_(0) {}

    // Push data to buffer, return false if full
    bool push(const std::string& data) {
        size_t current_tail = tail_.load(std::memory_order_relaxed);
        size_t next_tail = (current_tail + 1) % buffer_.size();
        if (next_tail == head_.load(std::memory_order_acquire)) {
            return false; // Buffer full
        }
        buffer_[current_tail] = data;
        tail_.store(next_tail, std::memory_order_release);
        return true;
    }

    // Pop data from buffer, return false if empty
    bool pop(std::string& data) {
        size_t current_head = head_.load(std::memory_order_relaxed);
        if (current_head == tail_.load(std::memory_order_acquire)) {
            return false; // Buffer empty
        }
        data = std::move(buffer_[current_head]);
        head_.store((current_head + 1) % buffer_.size(), std::memory_order_release);
        return true;
    }

private:
    std::vector<std::string> buffer_; // Dynamic array for data
    std::atomic<size_t> head_; // Read position
    std::atomic<size_t> tail_; // Write position
};

#endif // RING_BUFFER_H