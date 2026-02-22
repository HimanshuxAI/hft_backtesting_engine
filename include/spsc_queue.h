#pragma once

#include <atomic>
#include <cstddef>
#include <new>
#include <type_traits>

namespace hft {

// Lock-free SPSC ring buffer. Capacity must be power of 2.
// Cache-line padded head/tail, acquire/release semantics, no heap alloc after ctor.
template <typename T, size_t Capacity>
class SPSCQueue {
    static_assert((Capacity & (Capacity - 1)) == 0, "Capacity must be a power of two");
    static_assert(std::is_trivially_copyable_v<T>, "T must be trivially copyable");

public:
    SPSCQueue() : head_(0), tail_(0) {}

    SPSCQueue(const SPSCQueue&) = delete;
    SPSCQueue& operator=(const SPSCQueue&) = delete;

    // Returns false if full. Only call from producer thread.
    bool try_push(const T& item) noexcept {
        const size_t head = head_.load(std::memory_order_relaxed);
        const size_t next = (head + 1) & kMask;
        if (next == tail_.load(std::memory_order_acquire))
            return false;
        buffer_[head] = item;
        head_.store(next, std::memory_order_release);
        return true;
    }

    // Spin until space available
    void push(const T& item) noexcept {
        while (!try_push(item)) {
#if defined(__x86_64__) || defined(_M_X64)
            __builtin_ia32_pause();
#endif
        }
    }

    // Returns false if empty. Only call from consumer thread.
    bool try_pop(T& item) noexcept {
        const size_t tail = tail_.load(std::memory_order_relaxed);
        if (tail == head_.load(std::memory_order_acquire))
            return false;
        item = buffer_[tail];
        tail_.store((tail + 1) & kMask, std::memory_order_release);
        return true;
    }

    // Spin until data available
    T pop() noexcept {
        T item;
        while (!try_pop(item)) {
#if defined(__x86_64__) || defined(_M_X64)
            __builtin_ia32_pause();
#endif
        }
        return item;
    }

    size_t size_approx() const noexcept {
        return (head_.load(std::memory_order_relaxed) -
                tail_.load(std::memory_order_relaxed)) & kMask;
    }

    bool empty() const noexcept {
        return head_.load(std::memory_order_relaxed) ==
               tail_.load(std::memory_order_relaxed);
    }

private:
    static constexpr size_t kMask = Capacity - 1;

    // pad to separate cache lines (avoid false sharing)
    alignas(64) std::atomic<size_t> head_;
    char pad1_[64 - sizeof(std::atomic<size_t>)];

    alignas(64) std::atomic<size_t> tail_;
    char pad2_[64 - sizeof(std::atomic<size_t>)];

    alignas(64) T buffer_[Capacity];
};

} // namespace hft
