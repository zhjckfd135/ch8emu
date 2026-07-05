cmake_minimum_required(VERSION 3.10)

project(ch8emu C)

set(CMAKE_C_STANDART 11)
set(CMAKE_C_STANDART_REQUIRED ON)

add_executable(ch8emu 
	main.c
	platform_tools.c
)

target_include_directories(ch8emu PRIVATE
	${CMAKE_CURRENT_SOURCE_DIR}
)

if (WIN32)
	target_link_libraries(ch8emu PRIVATE winmm)
endif()

if (UNIX AND NOT APPLE)
	target_link_libraries(ch8emu PRIVATE asound)
endif()
