cmake_minimum_required(VERSION 3.5)
project(mechanoids-tools)

# Create directory 'bin' and move binaries into it after building ends
set(CMAKE_RUNTIME_OUTPUT_DIRECTORY ${CMAKE_SOURCE_DIR}/bin)

# Tools subprojects
add_subdirectory(src/name_generator)
add_subdirectory(src/unpaker)
