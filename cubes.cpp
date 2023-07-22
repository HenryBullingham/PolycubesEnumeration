#include "popl.hpp"

#include <cstdlib>
#include <chrono>
#include <vector>

#include "cubes.h"

int main(int argc, char** argv)
{
    popl::OptionParser options("Options");
    auto nOption = options.add<popl::Value<int>>("n", "N", "The number of cubes within each polycube");
    auto threadOption = options.add<popl::Value<int>>("t", "threads", "The number of worker threads to use");
    options.parse(argc, argv);

    if (!nOption->is_set())
    {
        printf("%s\n", options.help().c_str());
        return -1;
    }

    int n = nOption->value();
    auto t1_start =  std::chrono::high_resolution_clock::now();

    size_t polycubes;

    size_t num_threads = 1;
    if (threadOption->is_set())
    {
        num_threads = threadOption->value();
    }

    polycubes_thread_pool pool;
    pool.init(num_threads);
    polycubes = generate_polycubes_threaded(n, pool);
    pool.shutdown();

    auto t1_stop = std::chrono::high_resolution_clock::now();

    printf("Found %llu unique polycubes\n", polycubes);
    printf("Elapsed time: %f s \n", (std::chrono::duration_cast<std::chrono::milliseconds>(t1_stop - t1_start).count() / 1000.0f));

    return 0;
}