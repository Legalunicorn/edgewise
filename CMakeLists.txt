cmake_minimum_required(VERSION 3.20)
project(edgewise)

# GoogleTest configuration.
include(FetchContent)
FetchContent_Declare(
  googletest
  URL https://github.com/google/googletest/archive/03597a01ee50ed33e9dfd640b249b4be3799d395.zip
)
# For Windows: Prevent overriding the parent project's compiler/linker settings.
set(gtest_force_shared_crt ON CACHE BOOL "" FORCE)
FetchContent_MakeAvailable(googletest)

find_package(Threads REQUIRED) # Find the platform's threading requirements.

# Compile graph algorithms once for all executable targets.
add_library(edgewise_graph STATIC
    src/bfs_sequential.cpp
    src/bfs_parallel_v1.cpp
    src/bfs_parallel_v2.cpp
    src/bfs_parallel_v3.cpp
    src/bfs_parallel_v4.cpp
    src/pagerank_sequential.cpp
    src/pagerank_parallel.cpp
    src/cc_sequential_bfs.cpp
    src/cc_sequential_dsu.cpp
    src/cc_parallel_dsu.cpp
)
target_compile_features(edgewise_graph PUBLIC cxx_std_20) # Also applies to consumers.
target_include_directories(edgewise_graph PUBLIC src) # Expose graph.hpp to consumers.
target_link_libraries(edgewise_graph PUBLIC Threads::Threads) # Propagate thread support.

add_executable(edgewise src/main.cpp)
target_link_libraries(edgewise PRIVATE edgewise_graph) # Link the shared graph code.

enable_testing() # Enable CTest integration.

add_executable(bfs_test tests/bfs_tests.cpp)
target_link_libraries(bfs_test PRIVATE edgewise_graph GTest::gtest_main) # Graph code and test main.

include(GoogleTest) # Load CMake's GoogleTest integration.
gtest_discover_tests(bfs_test) # Register each named case with CTest.

add_executable(pagerank_test tests/pagerank_tests.cpp)
target_link_libraries(pagerank_test PRIVATE edgewise_graph GTest::gtest_main)
gtest_discover_tests(pagerank_test)

add_executable(cc_test tests/cc_tests.cpp)
target_link_libraries(cc_test PRIVATE edgewise_graph GTest::gtest_main)
gtest_discover_tests(cc_test)

add_executable(dsu_concurrent_test tests/dsu_concurrent_tests.cpp)
target_link_libraries(dsu_concurrent_test PRIVATE edgewise_graph GTest::gtest_main)
gtest_discover_tests(dsu_concurrent_test PROPERTIES TIMEOUT 10)

# Keep graph generation and timing in the separate benchmark program.
add_executable(bfs_bench
    benchmarks/bfs_bench.cpp
    benchmarks/generate_graph.cpp
)
target_link_libraries(bfs_bench PRIVATE edgewise_graph) # Reuse graph code and its requirements.

add_executable(pagerank_bench
    benchmarks/pagerank_bench.cpp
    benchmarks/generate_graph.cpp
)
target_link_libraries(pagerank_bench PRIVATE edgewise_graph)

add_executable(cc_bench
    benchmarks/cc_bench.cpp
    benchmarks/generate_graph.cpp
)
target_link_libraries(cc_bench PRIVATE edgewise_graph)
