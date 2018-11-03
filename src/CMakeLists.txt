cmake_minimum_required(VERSION 2.8)
#project(pmstool)
add_executable(${PROJECT_NAME}
       	GoogleDocs.cpp GoogleDocs.h 
	main.cpp 
	Pms.cpp Pms.h
        Properties.cpp Properties.h
	Mqtt.cpp Mqtt.h
	CliParser.h CliParser.cpp
)

target_link_libraries(${PROJECT_NAME} mosquittopp pthread)

#${CMAKE_CURRENT_BINARY_DIR}/

INSTALL(TARGETS ${PROJECT_NAME} RUNTIME DESTINATION bin COMPONENT Runtime)
