cmake_minimum_required (VERSION 2.8)

include (CTest)
add_subdirectory(/usr/src/gtest gtest)
set(CMAKE_AUTOMOC ON)
set(CMAKE_INCLUDE_CURRENT_DIR ON)

function(add_gmock_test target)
    add_executable(${target} ${ARGN})
    target_link_libraries(${target} gmock)

    add_test(${target} ${target})
endfunction()


add_subdirectory(unit)
add_subdirectory(integration)
add_subdirectory(system)

