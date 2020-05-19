//
// Created by Marius Meyer on 04.12.19.
//

#include "linpack_benchmark.hpp"

using namespace linpack;

/**
The program entry point
*/
int
main(int argc, char *argv[]) {
    // Setup benchmark
    auto bm = LinpackBenchmark(argc, argv);
    bool success = bm.executeBenchmark();
    if (success) {
        return 0;
    }
    else {
        return 1;
    }
}

