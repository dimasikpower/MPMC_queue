


build:
	g++ -std=c++17 -O2 -pthread -I include src/main.cpp -o mpmc_test

.PHONY: build