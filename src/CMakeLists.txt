cmake_minimum_required(VERSION 2.8)

add_executable(${PROJECT_NAME}
       	GoogleDocs.cpp GoogleDocs.h 
	main.cpp 
	Pms.cpp Pms.h
        Properties.cpp Properties.h
	Mqtt.cpp Mqtt.h
        )

    target_link_libraries(${PROJECT_NAME} mosquittopp)
