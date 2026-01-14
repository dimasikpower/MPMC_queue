# 🔥 Lock-Free MPMC Queue — 15ns Latency, 47M ops/sec

> *"If your queue blocks, you're not in HFT — you're in line for coffee."*

A **production-grade**, **cache-line optimized**, **non-blocking** Multi-Producer Multi-Consumer queue in pure C++17.  
Built for **high-frequency trading**, **low-latency systems**, and **people who measure time in nanoseconds**.

---

## 🚀 Benchmarks (x86_64, `-O3 -march=native`)

| Metric          | Result               |
|-----------------|----------------------|
| **p50 latency** | **15.6 ns**          |
| **p99 latency** | **34.4 ns**          |
| **Throughput**  | **47M ops/sec**      |
| **Max contention** | 2P + 2C on 2 cores |

✅ No locks  
✅ No syscalls (`yield` banned)  
✅ False-sharing protected (`alignas(64)`)  
✅ Correct memory ordering (`acquire`/`release`)  
✅ Ticket-turn cycle-based slot reuse  

---

## 💡 Why This Matters in HFT

- In HFT, **100 ns = lost money**.
- Blocking queues? → **latency spikes → P&L bleed**.
- This queue ensures **predictable, sub-50ns operations** even under contention.
- Used as a building block for:
  - Market data pipelines
  - Order book updates
  - Risk engine feeds

---

## 🧠 Core Idea: Ticket + Turn + Cycle

- Each producer/consumer gets a **unique ticket** via `fetch_add`.
- Slots are reused in cycles: `turn = 2 * (ticket / capacity)`.
- **Even turn** → slot free for writing  
  **Odd turn** → slot ready for reading  
- Prevents ABA and cross-cycle corruption **without garbage collection**.

---

## 🛠️ Build & Run

```bash
make bench_v2
taskset -c 2,3 ./bench_v2
```


### 💼 Open to Opportunities

If you’re hiring...

🎯 Target Roles
HFT Engineer (Jump, Citadel, Jane Street, Optiver, Akuna)
Low-Latency Systems Developer
Quant Developer (infrastructure side)
If you’re hiring and this code makes you sweat — let’s talk.
I don’t just write fast code. I write profitable code.

P.S. Yes, I know std::atomic isn’t enough for FPGA-based matching engines.
But it’s more than enough to get me a Porsche. 🏁