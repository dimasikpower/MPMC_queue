#include "../include/mpmc_queue.h"
#include <atomic>
#include <thread>
#include <vector>
#include <iostream>
#include <cassert>




// === ТЕСТ: 2 продюсера, 2 консьюмера ===
int main() {
    const size_t QUEUE_SIZE = 4;
    SimpleMPMCQueue<int> queue(QUEUE_SIZE);

    const int NUM_ITEMS = 10000;
    std::vector<std::thread> threads;

    // 2 продюсера
    for (int prod_id = 0; prod_id < 2; ++prod_id) {
        threads.emplace_back([&, prod_id]() {
            for (int i = 0; i < NUM_ITEMS; ++i) {
                int value = prod_id * 1000 + i;
                queue.push(value);
            }
        });
    }

    // 2 консьюмера
    std::atomic<int> total_popped{0};
    for (int cons_id = 0; cons_id < 2; ++cons_id) {
        threads.emplace_back([&, cons_id]() {
            int local_count = 0;
            while (total_popped.load() < 2 * NUM_ITEMS) {
                int value;
                if (queue.try_pop(value)) {      // ← try_pop, а не pop!
                    ++local_count;
                    ++total_popped;
                    // std::cout << "Consumer " << cons_id << " got: " << value << "\n";
                } else {
                    std::this_thread::yield();   // ← даём CPU другим потокам
                }
            }

            std::cout << "Consumer " << cons_id << " processed " << local_count << " items.\n";
        });
    }

    for (auto& t : threads) {
        t.join();
    }

    std::cout << "✅ All done! Total items: " << total_popped.load() << "\n";
    assert(total_popped == 2 * NUM_ITEMS);
    return 0;
}