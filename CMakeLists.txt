cmake_minimum_required(VERSION 3.11)

project(Vulkan)

set(CMAKE_EXPORT_COMPILE_COMMANDS true)

if(CMAKE_CXX_COMPILER_ID MATCHES "Clang")
    if(DEBUG)
        set(CMAKE_C_FLAGS "${CMAKE_C_FLAGS} -std=c11 -Og -g3 -fuse-ld=lld")
        set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -std=c++20 -stdlib=libc++ -Og -g3 -fuse-ld=lld -Wall")
        set(CMAKE_EXE_LINKER_FLAGS "${CMAKE_EXE_LINKER_FLAGS} -fuse-ld=lld")
    else()
        set(CMAKE_C_FLAGS "${CMAKE_C_FLAGS} -std=c11 -O3 -fuse-ld=lld -flto=full")
        set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -std=c++20 -stdlib=libc++ -O3 -fuse-ld=lld -flto=full")
        set(CMAKE_EXE_LINKER_FLAGS "${CMAKE_EXE_LINKER_FLAGS} -fuse-ld=lld -flto=full")
    endif(DEBUG)
else()
    set(CMAKE_CXX_STANDARD 20)
    set(CMAKE_CXX_STANDARD_REQUIRED true)
endif(CMAKE_CXX_COMPILER_ID MATCHES "Clang")

# GLFW
set(GLFW_BUILD_EXAMPLES OFF CACHE BOOL "GLFW lib only")
set(GLFW_BUILD_TESTS OFF CACHE BOOL "GLFW lib only")
set(GLFW_BUILD_DOCS OFF CACHE BOOL "GLFW lib only")
set(GLFW_BUILD_INSTALL OFF CACHE BOOL "GLFW lib only")
add_subdirectory(vendor/glfw)

add_executable(${PROJECT_NAME} main.cpp)

target_include_directories(${PROJECT_NAME}
    PRIVATE vendor
)
