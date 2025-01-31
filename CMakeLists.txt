cmake_minimum_required(VERSION 3.15)
project(AirMobiSimLibveins)

# Set C++ Standard
set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

# Conan-generated toolchain file
if(EXISTS "${CMAKE_BINARY_DIR}/generators/conan_toolchain.cmake")
    include("${CMAKE_BINARY_DIR}/generators/conan_toolchain.cmake")
else()
    message(FATAL_ERROR "Conan toolchain file not found. Please run 'conan install'.")
endif()

# Include Conan-generated configurations
find_package(gRPC REQUIRED)
find_package(Protobuf REQUIRED)

# Custom target for running src/Makefile
add_custom_target(build_src ALL
    COMMAND ${CMAKE_MAKE_PROGRAM} -C ${CMAKE_SOURCE_DIR}/src
    COMMENT "Building src/Makefile..."
)

# gRPC Include und Link für src
include_directories(${CONAN_INCLUDE_DIRS_GRPC})
link_directories(${CONAN_LIB_DIRS_GRPC})

