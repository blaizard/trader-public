# Minimal version for CMake
cmake_minimum_required(VERSION 3.1)

# Project's name
project(main)

# Set c++11 standard
set(CMAKE_CXX_STANDARD 11)

# Set the usefull path
set(ROOT_PATH ${CMAKE_CURRENT_SOURCE_DIR})

# Set the output folder where your program will be created
set(CMAKE_ARCHIVE_OUTPUT_DIRECTORY ${ROOT_PATH}/build/lib)
set(CMAKE_LIBRARY_OUTPUT_DIRECTORY ${ROOT_PATH}/build/lib)
set(CMAKE_RUNTIME_OUTPUT_DIRECTORY ${ROOT_PATH}/build/bin)

# Set default compilation flags
set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -O0 -rdynamic")
set(CMAKE_EXE_LINKER_FLAGS "${CMAKE_EXE_LINKER_FLAGS} -pthread")

# Include directory
include_directories(${ROOT_PATH})
include_directories("Exchange/rapidjson/include")

# Compile IrStd with the following flags
add_definitions(-DIRSTD_MEMORY_MONITOR=1)

# Add dependencies
add_subdirectory(IrStd)
add_subdirectory(Trader)

# Set the source files
set(sources main.cpp)

# Add main executable
add_executable(main ${sources})
target_link_libraries(main irstd atomic trader curl)

# Copy the content of the www directory into the runtime output
add_custom_target(www_target ALL
		COMMAND rm -f ${CMAKE_RUNTIME_OUTPUT_DIRECTORY}/www && ln -s ${ROOT_PATH}/www ${CMAKE_RUNTIME_OUTPUT_DIRECTORY}/www
		COMMENT "Generating symbolic link for www")
