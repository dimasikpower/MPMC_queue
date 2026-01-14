#include "../include/mpmc_queue.h"
#include <vector>
#include <thread>
#include <algorithm>
#include <iostream>
#include <atomic>
#include <cassert>
#include <immintrin.h>  // для _mm_pause()

// Точное чтение тактов CPU (x86)
static inline uint64_t rdtsc() {
    uint32_t lo, hi;
    __asm__ volatile ("rdtsc" : "=a" (lo), "=d" (hi));
    return ((uint64_t)hi << 32) | lo;
}

// Перевод тактов в наносекунды (примерно)
double cycles_to_ns(uint64_t cycles) {
    // Замени на частоту твоего CPU (в ГГц)
    const double CPU_GHZ = 3.2; // ← УТОЧНИ ДЛЯ СВОЕЙ МАШИНЫ!
    return static_cast<double>(cycles) / CPU_GHZ;
}

int main() {
    const size_t QUEUE_CAPACITY = 1024;
    const size_t OPERATIONS_PER_PRODUCER = 500'000;
    const int NUM_PRODUCERS = 2;
    const int NUM_CONSUMERS = 2;

    SimpleMPMCQueue<int> queue(QUEUE_CAPACITY);

    // Векторы для сбора latency (только продюсеры)
    std::vector<std::vector<uint64_t>> all_latencies(NUM_PRODUCERS);

    std::atomic<bool> start_flag{false};
    std::atomic<int> ready_count{0};

    std::vector<std::thread> threads;

    // === Продюсеры: меряют latency try_push ===
    for (int tid = 0; tid < NUM_PRODUCERS; ++tid) {
        threads.emplace_back([&, tid]() {
            ready_count.fetch_add(1);
            while (!start_flag.load()) {} // ждём старта

            std::vector<uint64_t> local_latencies;
            local_latencies.reserve(OPERATIONS_PER_PRODUCER);

            for (size_t i = 0; i < OPERATIONS_PER_PRODUCER; ++i) {
                int value = tid * OPERATIONS_PER_PRODUCER + i;
                while (true) {
                    auto t0 = rdtsc();
                    if (queue.try_push(value)) {
                        auto t1 = rdtsc();
                        local_latencies.push_back(t1 - t0);
                        break;
                    }
                    _mm_pause(); // активный спин, НЕ включаем в latency
                }
            }
            all_latencies[tid] = std::move(local_latencies);
        });
    }

    // === Консьюмеры: просто выгребают всё ===
    std::atomic<size_t> total_popped{0};
    const size_t TOTAL_ITEMS = NUM_PRODUCERS * OPERATIONS_PER_PRODUCER;

    for (int tid = 0; tid < NUM_CONSUMERS; ++tid) {
        threads.emplace_back([&]() {
            ready_count.fetch_add(1);
            while (!start_flag.load()) {}

            size_t local_count = 0;
            while (local_count < TOTAL_ITEMS / NUM_CONSUMERS) {
                int val;
                if (queue.try_pop(val)) {
                    ++local_count;
                    total_popped.fetch_add(1, std::memory_order_relaxed);
                } else {
                    _mm_pause();
                }
            }
        });
    }

    // Ждём, пока все потоки запустятся
    while (ready_count.load() < NUM_PRODUCERS + NUM_CONSUMERS) {
        std::this_thread::yield();
    }

    // СТАРТ!
    auto global_start = rdtsc();
    start_flag.store(true);

    for (auto& t : threads) {
        t.join();
    }
    auto global_end = rdtsc();

    // === Анализ latency ===
    std::vector<uint64_t> combined;
    for (auto& vec : all_latencies) {
        combined.insert(combined.end(), vec.begin(), vec.end());
    }

    std::sort(combined.begin(), combined.end());
    size_t n = combined.size();
    uint64_t p50 = combined[n / 2];
    uint64_t p99 = combined[n * 99 / 100];

    // === Throughput ===
    double total_time_ns = cycles_to_ns(global_end - global_start);
    double throughput = (TOTAL_ITEMS * 2) / (total_time_ns / 1e9); // ops/sec

    // === Вывод ===
    std::cout << "=== NON-BLOCKING BENCHMARK ===\n";
    std::cout << "Total items: " << TOTAL_ITEMS << "\n";
    std::cout << "Throughput: " << static_cast<long long>(throughput) << " ops/sec\n";
    std::cout << "Latency (p50): " << cycles_to_ns(p50) << " ns\n";
    std::cout << "Latency (p99): " << cycles_to_ns(p99) << " ns\n";
    std::cout << "Max latency: " << cycles_to_ns(combined.back()) << " ns\n";

    assert(total_popped == TOTAL_ITEMS);
}