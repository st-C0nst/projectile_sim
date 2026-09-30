cmake_minimum_required(VERSION 3.25)

project(projectile_system LANGUAGES CXX)

add_library(projectile_sim INTERFACE)
target_include_directories(projectile_sim INTERFACE "${CMAKE_CURRENT_SOURCE_DIR}/include")
target_compile_features(projectile_sim INTERFACE cxx_std_23)

add_executable(projectile_system benchmarks/main.cpp)
target_link_libraries(projectile_system PRIVATE projectile_sim)
target_compile_options(projectile_system PRIVATE -Wall -Wextra -Wpedantic)

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
