cmake_minimum_required(VERSION 3.22)
project(semaphore_koans CXX)

set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)

if(NOT CMAKE_BUILD_TYPE)
  set(CMAKE_BUILD_TYPE RelWithDebInfo)
endif()

find_package(Threads REQUIRED)
enable_testing()

# Every directory under koans/ that contains a test_*.cpp becomes one test
# executable named after the directory (e.g. koan target "02_rendezvous").
file(GLOB koan_dirs RELATIVE ${CMAKE_CURRENT_SOURCE_DIR}/koans ${CMAKE_CURRENT_SOURCE_DIR}/koans/*)
foreach(dir ${koan_dirs})
  file(GLOB test_sources ${CMAKE_CURRENT_SOURCE_DIR}/koans/${dir}/test_*.cpp)
  if(test_sources)
    add_executable(${dir} ${test_sources})
    target_include_directories(${dir} PRIVATE
      ${CMAKE_CURRENT_SOURCE_DIR}/common
      ${CMAKE_CURRENT_SOURCE_DIR}/koans/${dir})
    target_link_libraries(${dir} PRIVATE Threads::Threads)
    target_compile_options(${dir} PRIVATE -Wall -Wextra)
    add_test(NAME ${dir} COMMAND ${dir})
    set_tests_properties(${dir} PROPERTIES TIMEOUT 120)
  endif()
endforeach()
