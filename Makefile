


build:
	g++ -std=c++17 -O2 -pthread -I include src/main.cpp -o mpmc_test

.PHONY: build bench_v2


bench:
	g++ -std=c++17 -O2 -pthread -I include src/benchmark.cpp -o bench_test

bench_v2:
	g++ -O3 -march=native -DNDEBUG -pthread src/benchmark.cpp -o bench_v2