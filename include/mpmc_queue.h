#include <atomic>
#include <thread>
#include <vector>
#include <iostream>
#include <cassert>

template<typename T>
class SimpleMPMCQueue {
private:
    struct Slot {
        T data;
        std::atomic<size_t> turn{0}; // чётное = можно писать, нечётное = можно читать
    };

    size_t capacity_;
    std::vector<Slot> buffer_;
    alignas(64) std::atomic<size_t> head_{0}; // ticket для записи
    char pad0[64];
    alignas(64) std::atomic<size_t> tail_{0}; // ticket для чтения
    char pad1[64];

public:
    explicit SimpleMPMCQueue(size_t capacity) 
        : capacity_(capacity), buffer_(capacity) {}

    void push(T value) {
        size_t ticket = head_.fetch_add(1, std::memory_order_relaxed);
        size_t slot_idx = ticket % capacity_;
        size_t expected_turn = 2 * (ticket / capacity_);

        // Ждём, пока слот освободится для записи
        while (buffer_[slot_idx].turn.load(std::memory_order_acquire) != expected_turn) {
            std::this_thread::yield(); // уступаем CPU
        }

        buffer_[slot_idx].data = value;
        // Говорим: "теперь можно читать"
        buffer_[slot_idx].turn.store(expected_turn + 1, std::memory_order_release);
    }

    bool try_push(T value) {
        size_t current_head = head_.load(std::memory_order_acquire);
        size_t slot_idx = current_head % capacity_;
        size_t expected_turn = 2 * (current_head / capacity_);

        if (buffer_[slot_idx].turn.load(std::memory_order_acquire) != expected_turn) {
            return false; // очередь полна
        }

        if (!head_.compare_exchange_strong(
                current_head,
                current_head + 1,
                std::memory_order_release,
                std::memory_order_acquire)) {
            return false;
        }

        buffer_[slot_idx].data = value;
        buffer_[slot_idx].turn.store(expected_turn + 1, std::memory_order_release);
        return true;
    }

    bool try_pop(T& out) {
        size_t current_tail = tail_.load(std::memory_order_acquire); // ← acquire здесь важно!
        size_t slot_idx = current_tail % capacity_;
        size_t expected_turn = 2 * (current_tail / capacity_) + 1;

        // Проверяем готовность
        if (buffer_[slot_idx].turn.load(std::memory_order_acquire) != expected_turn) {
            return false;
        }

        // Пробуем захватить этот ticket
        if (tail_.compare_exchange_strong(
                current_tail, 
                current_tail + 1,
                std::memory_order_release,   // ← успех: release, потому что двигаем tail
                std::memory_order_acquire    // ← неудача: нужно свежее значение
            )) {
            out = buffer_[slot_idx].data;
            buffer_[slot_idx].turn.store(expected_turn + 1, std::memory_order_release);
            return true;
        }
        return false;
    }
};