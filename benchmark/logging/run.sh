#!/usr/bin/env bash

set -e

# sudo apt-get update && sudo apt-get install libspdlog-dev
echo "Building..."
g++ -std=c++11 -Wall -Wextra -Werror -Wconversion -Wshadow -O3 -o benchmark_logging benchmark/logging/main.cc src/*.cc src/util/*.cc -Iinclude -lspdlog -lfmt
echo "Starting benchmarking..."
echo ""
./benchmark_logging
rm ./benchmark_logging
echo ""
echo "Benchmarking completed. ✔️"
