#include "catch_amalgamated.hpp"

#include "cubes.h"

#include <utility>

//I realize that there should be more unit tests - there were in a previous iteration of the code, 
// but because of the major changes involved, i ended up just verifying the correctness by checking the output

TEST_CASE("CHECK THAT DFS expansion is correct")
{
    //Expected values obtained from: https://oeis.org/A000162
    std::pair<int, uint64_t> n_cubes_pair = GENERATE(
        std::make_pair<int, uint64_t>(1, 1LLu),
        std::make_pair<int, uint64_t>(2, 1LLu),
        std::make_pair<int, uint64_t>(3, 2LLu),
        std::make_pair<int, uint64_t>(4, 8LLu),
        std::make_pair<int, uint64_t>(5, 29LLu),
        std::make_pair<int, uint64_t>(6, 166LLu),
        std::make_pair<int, uint64_t>(7, 1023Lu),
        std::make_pair<int, uint64_t>(8, 6922LLu)
        );

    stack_allocator allocator;
    uint64_t result = expand_polycubes_dfs(allocator, n_cubes_pair.first, n_cubes_pair.first, [](auto&&) {}, [](auto&&) {});

    REQUIRE(result == n_cubes_pair.second);
}