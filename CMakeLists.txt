cmake_minimum_required(VERSION 3.7)

project(emu
	VERSION 1.0
	LANGUAGES CXX
)


# Lib
add_library(libchip8
	chip8.cpp
	chip8.h
)

target_compile_features(libchip8
	PUBLIC
	cxx_std_17
)


# Exe
add_executable(chip8
	main.cpp
)

target_link_libraries(chip8
	PRIVATE
	libchip8
)
