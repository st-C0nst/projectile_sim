cmake_minimum_required(VERSION 3.25)

project(projectile_system LANGUAGES CXX)

add_executable(projectile_system src/main.cpp)

target_compile_features(projectile_system PRIVATE cxx_std_23)

target_compile_options(projectile_system PRIVATE -Wall -Wextra -Wpendantic)
