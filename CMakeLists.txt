cmake_minimum_required(VERSION 3.11)

project(Vulkan)

set(CMAKE_EXPORT_COMPILE_COMMANDS true)

if(CMAKE_CXX_COMPILER_ID MATCHES "Clang")
    set(CMAKE_C_FLAGS "${CMAKE_C_FLAGS} -std=c11 -fuse-ld=lld -Wno-unused-command-line-argument")
    set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -std=c++20 -stdlib=libc++ -fuse-ld=lld -Wno-unused-command-line-argument")
    set(CMAKE_EXE_LINKER_FLAGS "${CMAKE_EXE_LINKER_FLAGS} -fuse-ld=lld")
    if(CMAKE_BUILD_TYPE MATCHES Debug)
        # TODO: Check -g3 -g difference
        set(CMAKE_C_FLAGS_DEBUG     "${CMAKE_C_FLAGS_DEBUG} --optimize=g --debug=3 -gdwarf-4 -glldb -fstandalone-debug -fdebug-info-for-profiling -fdebug-macro -fdebug-default-version=4 -fno-omit-frame-pointer -fno-optimize-sibling-calls")
        set(CMAKE_CXX_FLAGS_DEBUG "${CMAKE_CXX_FLAGS_DEBUG} --optimize=g --debug=3 -gdwarf-4 -glldb -fstandalone-debug -fdebug-info-for-profiling -fdebug-macro -fdebug-default-version=4 -fno-omit-frame-pointer -fno-optimize-sibling-calls -Wall -Wextra -Weverything -Wpedantic -Wno-unused-parameter -Wno-c++98-compat -Wno-c++98-compat-pedantic -Wno-missing-prototypes -Wno-global-constructors -Wno-exit-time-destructors -Wno-shadow -Wno-shorten-64-to-32 -Wno-documentation -Wno-documentation-unknown-command")
    else()
        set(CMAKE_C_FLAGS_RELEASE "${CMAKE_C_FLAGS_RELEASE} --optimize=3 -flto -fwhole-program-vtables")
        set(CMAKE_CXX_FLAGS_RELEASE "${CMAKE_CXX_FLAGS_RELEASE} --optimize=3 -flto -fwhole-program-vtables")
        set(CMAKE_EXE_LINKER_FLAGS_RELEASE "${CMAKE_EXE_LINKER_FLAGS_RELEASE} -flto -fwhole-program-vtables")
    endif(CMAKE_BUILD_TYPE MATCHES Debug)
else()
    set(CMAKE_CXX_STANDARD 20)
    set(CMAKE_CXX_STANDARD_REQUIRED true)
endif(CMAKE_CXX_COMPILER_ID MATCHES "Clang")

add_executable(${PROJECT_NAME} main.cpp)

find_package(Vulkan REQUIRED)
set(VK_LAYER_PATH "$ENV{VULKAN_SDK}/etc/vulkan/explicit_layer.d")

# GLFW
set(GLFW_BUILD_EXAMPLES OFF CACHE BOOL "GLFW lib only")
set(GLFW_BUILD_TESTS OFF CACHE BOOL "GLFW lib only")
set(GLFW_BUILD_DOCS OFF CACHE BOOL "GLFW lib only")
set(GLFW_BUILD_INSTALL OFF CACHE BOOL "GLFW lib only")
add_subdirectory(vendor/glfw)

target_include_directories(${PROJECT_NAME}
    SYSTEM
    PRIVATE
        vendor/glm
    PUBLIC
        "$ENV{VULKAN_SDK}/include"
)

# target_link_directories(${PROJECT_NAME} PRIVATE $ENV{VULKAN_SDK}/lib)
target_link_libraries(${PROJECT_NAME} glfw ${GLFW_LIBRARIES} Vulkan::Vulkan)

# -debug-info-kind=standalone
# -dwarf-version=5
# -debugger-tuning=lldb
# -fsanitize=address 
# -fsanitize=undefined -fsanitize=array-bounds -fsanitize=null -fsanitize=object-size -fsanitize=pointer-overflow
