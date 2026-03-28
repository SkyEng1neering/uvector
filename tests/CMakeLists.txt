cmake_minimum_required(VERSION 3.14)
project(uvector_tests)

set(CMAKE_CXX_STANDARD 14)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

# Fetch Google Test
include(FetchContent)
FetchContent_Declare(
    googletest
    GIT_REPOSITORY https://github.com/google/googletest.git
    GIT_TAG v1.17.0
)
set(gtest_force_shared_crt ON CACHE BOOL "" FORCE)
FetchContent_MakeAvailable(googletest)

# Compiler flags
add_compile_options(-Wall -Wextra -g)

# Sanitizers (optional but recommended)
option(USE_SANITIZERS "Enable ASan/UBSan" ON)
if(USE_SANITIZERS AND NOT WIN32)
    add_compile_options(-fsanitize=address,undefined)
    add_link_options(-fsanitize=address,undefined)
endif()

# ==================== Multi-Heap Mode Tests ====================

# dalloc library for multi-heap mode
add_library(dalloc_multiheap STATIC ../../dalloc/src/dalloc.c)
target_include_directories(dalloc_multiheap PUBLIC
    ${CMAKE_CURRENT_SOURCE_DIR}/../../dalloc/inc
    ${CMAKE_CURRENT_SOURCE_DIR}
)
target_compile_definitions(dalloc_multiheap PUBLIC
    DALLOC_CUSTOM_CONF_FILE="uvector_test_conf.h"
)

# Multi-heap test sources
set(MULTIHEAP_TEST_SOURCES
    test_basic.cpp
    test_element_access.cpp
    test_capacity.cpp
    test_modifiers.cpp
    test_copy_semantics.cpp
    test_move_semantics.cpp
    test_type_traits.cpp
    test_edge_cases.cpp
    test_stress.cpp
    test_integrity.cpp
    test_nested_containers.cpp
)

# Multi-heap test executable
add_executable(uvector_tests ${MULTIHEAP_TEST_SOURCES})
target_link_libraries(uvector_tests
    dalloc_multiheap
    GTest::gtest_main
)
target_include_directories(uvector_tests PRIVATE
    ${CMAKE_CURRENT_SOURCE_DIR}/../inc
    ${CMAKE_CURRENT_SOURCE_DIR}/../../dalloc/inc
    ${CMAKE_CURRENT_SOURCE_DIR}
)

# ==================== Single-Heap Mode Tests ====================

# dalloc library for single-heap mode
add_library(dalloc_singleheap STATIC ../../dalloc/src/dalloc.c)
target_include_directories(dalloc_singleheap PUBLIC
    ${CMAKE_CURRENT_SOURCE_DIR}/../../dalloc/inc
    ${CMAKE_CURRENT_SOURCE_DIR}
)
target_compile_definitions(dalloc_singleheap PUBLIC
    USE_SINGLE_HEAP_MEMORY
    DALLOC_CUSTOM_CONF_FILE="uvector_test_conf.h"
)

# Single-heap test executable
add_executable(uvector_single_heap_tests test_single_heap.cpp)
target_link_libraries(uvector_single_heap_tests
    dalloc_singleheap
    GTest::gtest_main
)
target_include_directories(uvector_single_heap_tests PRIVATE
    ${CMAKE_CURRENT_SOURCE_DIR}/../inc
    ${CMAKE_CURRENT_SOURCE_DIR}/../../dalloc/inc
    ${CMAKE_CURRENT_SOURCE_DIR}
)
target_compile_definitions(uvector_single_heap_tests PRIVATE
    USE_SINGLE_HEAP_MEMORY
)

# ==================== Thread Safety Tests ====================

# dalloc library with thread safety enabled (recursive pthread mutex)
add_library(dalloc_threadsafe STATIC ../../dalloc/src/dalloc.c)
target_include_directories(dalloc_threadsafe PUBLIC
    ${CMAKE_CURRENT_SOURCE_DIR}/../../dalloc/inc
    ${CMAKE_CURRENT_SOURCE_DIR}
)
target_compile_definitions(dalloc_threadsafe PUBLIC
    DALLOC_CUSTOM_CONF_FILE="uvector_pthread_conf.h"
)

# Thread safety test executable
add_executable(uvector_thread_tests test_thread_safety.cpp)
target_link_libraries(uvector_thread_tests
    dalloc_threadsafe
    GTest::gtest_main
    pthread
)
target_include_directories(uvector_thread_tests PRIVATE
    ${CMAKE_CURRENT_SOURCE_DIR}/../inc
    ${CMAKE_CURRENT_SOURCE_DIR}/../../dalloc/inc
    ${CMAKE_CURRENT_SOURCE_DIR}
)

# Enable CTest integration
include(GoogleTest)
gtest_discover_tests(uvector_tests)
gtest_discover_tests(uvector_single_heap_tests)
gtest_discover_tests(uvector_thread_tests)
