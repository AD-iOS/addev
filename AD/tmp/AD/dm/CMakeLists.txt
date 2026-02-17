cmake_minimum_required(VERSION 3.13)
project(dm)

set(CMAKE_C_COMPILER "/bin/clang")
set(CMAKE_CXX_COMPILER "/bin/clang++")

enable_language(OBJC)
set(CMAKE_OBJC_COMPILER "/bin/clang")

set(CMAKE_OSX_SYSROOT "/var/theos/sdks/iPhoneOS17.5.sdk")

add_executable(dm dm.cc)
target_include_directories(dm PRIVATE incline AD)

set(EXECUTABLE_OUTPUT_PATH ${CMAKE_BINARY_DIR}/bin)
