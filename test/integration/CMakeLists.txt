cmake_minimum_required (VERSION 2.8)

project(pms-int-test)


include_directories(../../src)
add_executable(${PROJECT_NAME}
    main.cpp
    PmsMock.h PmsMock.cpp
    /usr/src/gtest/src/gtest_main.cc)
target_link_libraries(${PROJECT_NAME} gtest pmscommon)

add_test(NAME IntTest COMMAND ${PROJECT_NAME})
install(TARGETS ${PROJECT_NAME} DESTINATION bin COMPONENT Test)

