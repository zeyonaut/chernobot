cmake_minimum_required(VERSION 3.7.2)
project(Chernobot)

set(CMAKE_CXX_STANDARD 17)

# Find cmake modules in ./cmake
set(CMAKE_MODULE_PATH ${CMAKE_MODULE_PATH} "${Chernobot_SOURCE_DIR}/cmake")
# Set binary folder to ./bin
set(BIN_DIR ${Chernobot_SOURCE_DIR}/bin)

set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -std=c++11")
# Set warning levels and debug/release specific flags
if (${CMAKE_CXX_COMPILER_ID} STREQUAL "GNU" OR ${CMAKE_CXX_COMPILER_ID} STREQUAL "Clang")
	set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -std=c++11")
	set(CMAKE_CXX_FLAGS_DEBUG "${CMAKE_CXX_FLAGS} ${CMAKE_CXX_FLAGS_DEBUG} -g")
	set(CMAKE_CXX_FLAGS_RELEASE "${CMAKE_CXX_FLAGS} ${CMAKE_CXX_FLAGS_RELEASE} -O2")
elseif (${CMAKE_CXX_COMPILER_ID} STREQUAL "MSVC")
	if (CMAKE_CXX_FLAGS MATCHES "/W[0-4]")
		string(REGEX REPLACE "/W[0-4]" "/W4" CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS}")
	else()
		set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} /W4")
	endif()
endif()

find_package(SDL2 REQUIRED)

# Build executable
include_directories(${Boost_INCLUDE_DIR})
include_directories(${SDL2_INCLUDE_DIR})
include_directories(${Chernobot_SOURCE_DIR}/ext/glad/include)
include_directories(${Chernobot_SOURCE_DIR}/ext/imgui)
include_directories(${Chernobot_SOURCE_DIR}/ext/serial/include)
include_directories(${Chernobot_SOURCE_DIR}/ext)
include_directories(${Chernobot_SOURCE_DIR}/ext/expected/tl)

add_executable(Chernobot src/main.cpp)

file(GLOB_RECURSE Chernobot_SRC "src/*.cpp")
target_sources(Chernobot PUBLIC ${Chernobot_SRC})
file(GLOB Imgui_SRC "ext/imgui/*.cpp")
target_sources(Chernobot PUBLIC ${Imgui_SRC})
file(GLOB_RECURSE Serial_SRC "ext/serial/src/*.cc")
target_sources(Chernobot PUBLIC ${Serial_SRC})
file(GLOB_RECURSE Qfs_SRC "ext/qfs-cpp/*.cpp")
target_sources(Chernobot PUBLIC ${Qfs_SRC})
target_sources(Chernobot PUBLIC ${Chernobot_SOURCE_DIR}/ext/glad/src/glad.c)

target_link_libraries(Chernobot ${SDL2_LIBRARY})

#dependencies for serial lib - replace with rs232 also incomplete for linux and windows
if(APPLE)
	find_library(IOKIT_LIBRARY IOKit)
	find_library(FOUNDATION_LIBRARY Foundation)
	target_link_libraries(Chernobot ${FOUNDATION_LIBRARY} ${IOKIT_LIBRARY})
endif()


install(TARGETS Chernobot RUNTIME DESTINATION ${BIN_DIR})
