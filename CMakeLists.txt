CMAKE_MINIMUM_REQUIRED(VERSION 3.10 FATAL_ERROR)
project(ccomp)

set(CMAKE_CXX_STANDARD 20)
set(CMAKE_EXPORT_COMPILE_COMMANDS ON)

add_compile_options(
  "-Wall" "-Wpedantic" "-Wextra" "-fexceptions")

include_directories(${PROJECT_SOURCE_DIR}/include)

add_subdirectory(lib)
add_executable(ccomp main.cc)
target_link_libraries(ccomp ccomplib)
