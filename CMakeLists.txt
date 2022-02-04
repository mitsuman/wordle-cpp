project(wordle)
cmake_minimum_required(VERSION 3.13)

set(default_build_type "Release")

set(SOURCES
	main.cpp
)

add_executable(wordle ${SOURCES})
#target_link_libraries(wordle pthread)

