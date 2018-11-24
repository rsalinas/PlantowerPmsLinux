cmake_minimum_required(VERSION 2.8)
#project(pmstool)

add_library(pmscommon
    GoogleDocs.cpp GoogleDocs.h
    Pms.cpp Pms.h
    Properties.cpp Properties.h
    Mqtt.cpp Mqtt.h
    CliParser.h CliParser.cpp
    SignalSetter.h
    Process.cpp Process.h
    )
add_executable(${PROJECT_NAME}
    main.cpp
)

target_link_libraries(${PROJECT_NAME} mosquittopp pthread pmscommon)

#${CMAKE_CURRENT_BINARY_DIR}/

INSTALL(TARGETS ${PROJECT_NAME} RUNTIME DESTINATION bin COMPONENT Runtime)
