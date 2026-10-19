cmake_minimum_required(VERSION 3.20)
project(edgewise)

# GoogleTest confiration
include(FetchContent)
FetchContent_Declare(
  googletest
  URL https://github.com/google/googletest/archive/03597a01ee50ed33e9dfd640b249b4be3799d395.zip
)
# For Windows: Prevent overriding the parent project's compiler/linker settings
set(gtest_force_shared_crt ON CACHE BOOL "" FORCE)
FetchContent_MakeAvailable(googletest)

add_executable(edgewise src/main.cpp)
target_compile_features(edgewise PRIVATE cxx_std_20)
# find graph.hpp
target_include_directories(edgewise PRIVATE src) 


enable_testing() # Enable CTest integration.

add_executable(bfs_test tests/bfs_tests.cpp) # Build your test executable.
target_compile_features(bfs_test PRIVATE cxx_std_20) # Use C++20.
target_include_directories(bfs_test PRIVATE src) # Find graph.hpp.
target_link_libraries(bfs_test PRIVATE GTest::gtest_main) # Supply GoogleTest and its main().

include(GoogleTest) # Load CMake's GoogleTest integration.
gtest_discover_tests(bfs_test) # Register each named case with CTest.


# Build the benchmark as a separate program with its own main().
add_executable(bfs_bench
    benchmarks/bfs_bench.cpp # Timing, correctness comparisons, and CSV output.
    benchmarks/generate_graph.cpp # Compile the generator implementations too.
)
target_compile_features(bfs_bench PRIVATE cxx_std_20) # std::jthread requires C++20.
target_include_directories(bfs_bench PRIVATE src) # Locate the existing graph.hpp.
find_package(Threads REQUIRED) # Find this platform's C++ threading requirements.
target_link_libraries(bfs_bench PRIVATE Threads::Threads) # Apply those requirements to the benchmark.
