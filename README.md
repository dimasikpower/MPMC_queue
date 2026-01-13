# MPMC Ring Buffer (Multi-Producer Multi-Consumer Queue)

A lock-free bounded concurrent queue implemented from scratch in C++17,  
to deeply understand low-latency inter-thread communication for HFT systems.

## Features
- Lock-free design using atomic ticketing
- Cache-line awareness (planned)
- Supports multiple producers and consumers without mutexes
- Based on the turn-based algorithm (inspired by Rigtorp, Vyukov)

## Why?
Building this to prepare for high-frequency trading roles where nanosecond latency matters.

## Status
🚧 Work in progress — initial version coming soon.