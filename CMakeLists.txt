cmake_minimum_required(VERSION 3.10)
project(ParticleSimulation)

set(CMAKE_CXX_STANDARD 17)

# Add source and include directories
include_directories(include)
add_executable(ParticleSimulation src/main.cpp)
