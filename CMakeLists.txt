cmake_minimum_required(VERSION 3.20)
project(edgewise)
add_executable(edgewise src/main.cpp)
target_compile_features(edgewise PRIVATE cxx_std_20)
