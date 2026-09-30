cmake_minimum_required(VERSION 3.25)

project(projectile_system LANGUAGES CXX)

add_library(projectile_sim INTERFACE)
target_include_directories(projectile_sim INTERFACE "${CMAKE_CURRENT_SOURCE_DIR}/include")
target_compile_features(projectile_sim INTERFACE cxx_std_23)

option(BUILD_BENCHMARKS "Build projectile performance benchmarks" ON)
if(BUILD_BENCHMARKS)
    find_package(benchmark 1.9.4 CONFIG QUIET)
    if(NOT benchmark_FOUND)
        include(FetchContent)
        FetchContent_Declare(googlebenchmark
            URL https://github.com/google/benchmark/archive/refs/tags/v1.9.4.tar.gz
            URL_HASH SHA256=b334658edd35efcf06a99d9be21e4e93e092bd5f95074c1673d5c8705d95c104
            DOWNLOAD_EXTRACT_TIMESTAMP TRUE
        )
        set(BENCHMARK_ENABLE_TESTING OFF CACHE BOOL "" FORCE)
        set(BENCHMARK_ENABLE_INSTALL OFF CACHE BOOL "" FORCE)
        FetchContent_MakeAvailable(googlebenchmark)
        # Clang 22 diagnoses the upstream __COUNTER__ feature probe.
        if(CMAKE_CXX_COMPILER_ID MATCHES "Clang")
            include(CheckCXXCompilerFlag)
            check_cxx_compiler_flag("-Wno-c2y-extensions" HAVE_NO_C2Y_EXTENSIONS)
            if(HAVE_NO_C2Y_EXTENSIONS)
                target_compile_options(benchmark PRIVATE -Wno-c2y-extensions)
                target_compile_options(benchmark_main PRIVATE -Wno-c2y-extensions)
            endif()
        endif()
        set_property(TARGET benchmark PROPERTY SYSTEM TRUE)
    endif()
    add_executable(projectile_system benchmarks/main.cpp)
    target_link_libraries(projectile_system PRIVATE projectile_sim benchmark::benchmark)
    target_compile_options(projectile_system PRIVATE -Wall -Wextra -Wpedantic)
    target_compile_definitions(projectile_system PRIVATE
        PROJECTILE_COMPILER="${CMAKE_CXX_COMPILER_ID} ${CMAKE_CXX_COMPILER_VERSION}"
        PROJECTILE_BUILD_CONFIG="$<CONFIG>"
    )
endif()

include(CTest)
if(BUILD_TESTING)
    find_package(GTest CONFIG QUIET)
    if(NOT GTest_FOUND)
        include(FetchContent)
        FetchContent_Declare(googletest
            URL https://github.com/google/googletest/archive/refs/tags/v1.17.0.tar.gz
            URL_HASH SHA256=65fab701d9829d38cb77c14acdc431d2108bfdbf8979e40eb8ae567edf10b27c
            DOWNLOAD_EXTRACT_TIMESTAMP TRUE
        )
        set(INSTALL_GTEST OFF CACHE BOOL "" FORCE)
        set(BUILD_GMOCK OFF CACHE BOOL "" FORCE)
        FetchContent_MakeAvailable(googletest)
    endif()
    add_executable(projectile_tests tests/projectile_tests.cpp)
    target_link_libraries(projectile_tests PRIVATE projectile_sim GTest::gtest_main)
    target_compile_options(projectile_tests PRIVATE -Wall -Wextra -Wpedantic)
    include(GoogleTest)
    gtest_discover_tests(projectile_tests)
endif()
