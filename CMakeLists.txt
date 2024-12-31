cmake_minimum_required(VERSION 3.10)
project(ParticleSimulation)

# Set C++ standard
set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

# Include GLFW
include_directories(/glfw-3.4/include)

# Find and link OpenGL
find_package(OpenGL REQUIRED)

# Add source and include directories
include_directories(include)

# Define the executable
add_executable(ParticleSimulation src/main.cpp glad/src/glad.c)

target_include_directories(ParticleSimulation PUBLIC glad/include)

# Link libraries to the executable
target_link_libraries(ParticleSimulation PRIVATE glfw-3.4/lib/glfw3 OpenGL::GL)
