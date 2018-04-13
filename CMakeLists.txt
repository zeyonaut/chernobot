cmake_minimum_required(VERSION 3.7.2)
project(Chernobot)

# Find cmake modules in ./cmake
set(CMAKE_MODULE_PATH ${CMAKE_MODULE_PATH} "${Chernobot_SOURCE_DIR}/cmake")
# Set binary folder to ./bin
set(BIN_DIR ${Chernobot_SOURCE_DIR}/bin)

# Set warning levels and debug/release specific flags
if (${CMAKE_CXX_COMPILER_ID} STREQUAL "GNU" OR ${CMAKE_CXX_COMPILER_ID} STREQUAL "Clang")
	set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -std=c++14 -Wc++11-extensions")
	set(CMAKE_CXX_FLAGS_DEBUG "${CMAKE_CXX_FLAGS} ${CMAKE_CXX_FLAGS_DEBUG} -g")
	set(CMAKE_CXX_FLAGS_RELEASE "${CMAKE_CXX_FLAGS} ${CMAKE_CXX_FLAGS_RELEASE} -O2")
elseif (${CMAKE_CXX_COMPILER_ID} STREQUAL "MSVC")
	if (CMAKE_CXX_FLAGS MATCHES "/W[0-4]")
		string(REGEX REPLACE "/W[0-4]" "/W4" CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS}")
	else()
		set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} /W4")
	endif()
endif()

# Look up GLFW
find_package(GLFW REQUIRED)

# Build executable
include_directories(${Boost_INCLUDE_DIR})
include_directories(${GLFW_INCLUDE_DIR})
include_directories(${Chernobot_SOURCE_DIR}/ext/glad/include)
include_directories(${Chernobot_SOURCE_DIR}/ext/imgui)
include_directories(${Chernobot_SOURCE_DIR}/ext/serial/include)

add_executable(Chernobot src/main.cpp)

file(GLOB Chernobot_SRC "*.cpp")
target_sources(Chernobot PUBLIC ${Chernobot_SRC})
file(GLOB Nimir_SRC "src/nimir/*.cpp")
target_sources(Chernobot PUBLIC ${Nimir_SRC})
file(GLOB Imgui_SRC "ext/imgui/*.cpp")
target_sources(Chernobot PUBLIC ${Imgui_SRC})

file(GLOB_RECURSE Serial_SRC "ext/serial/src/*.cc")
target_sources(Chernobot PUBLIC ${Serial_SRC})
target_sources(Chernobot PUBLIC ${Chernobot_SOURCE_DIR}/ext/glad/src/glad.c)

target_link_libraries(Chernobot ${GLFW_LIBRARIES} ${QT_LIBRARIES})

install(TARGETS Chernobot RUNTIME DESTINATION ${BIN_DIR})
