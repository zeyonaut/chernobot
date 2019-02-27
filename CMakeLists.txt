cmake_minimum_required(VERSION 3.13)

project(Chernobot)

set(CMAKE_CXX_STANDARD 17)

set(CMAKE_MODULE_PATH ${CMAKE_MODULE_PATH} "${Chernobot_SOURCE_DIR}/cmake")

set(BIN_DIR ${Chernobot_SOURCE_DIR}/bin)

if(APPLE)
	if (EXISTS "/usr/local/bin/clang")
		set(CMAKE_CXX_COMPILER "/usr/local/bin/clang")
		include_directories("/usr/local/include")
	endif()
endif()

# Set warning levels and debug/release specific flags
if (${CMAKE_CXX_COMPILER_ID} STREQUAL "GNU" OR ${CMAKE_CXX_COMPILER_ID} STREQUAL "Clang")
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
find_package(FFMPEG REQUIRED)

# Build executable
include_directories(${Boost_INCLUDE_DIR})
include_directories(${SDL2_INCLUDE_DIR})
include_directories(${Chernobot_SOURCE_DIR}/ext/glad/include)
include_directories(${Chernobot_SOURCE_DIR}/ext/imgui)
include_directories(${Chernobot_SOURCE_DIR}/ext/serial/include)
include_directories(${Chernobot_SOURCE_DIR}/ext)
include_directories(${Chernobot_SOURCE_DIR}/ext/expected/tl)
include_directories(${Chernobot_SOURCE_DIR}/ext/stb)

add_executable(Chernobot src/main.cpp)

if(APPLE)
	add_definitions(-x objective-c++)
	set(CMAKE_EXE_LINKER_FLAGS "-framework Foundation -framework AVFoundation -w")

	set(
		PLATFORM_FILES
		src/advd/streamref_avfoundation.mm
	)
endif()


target_sources(Chernobot PUBLIC ${PLATFORM_FILES})

find_path(AVCODEC_INCLUDE_DIR libavcodec/avcodec.h)
find_path(AVFORMAT_INCLUDE_DIR libavformat/avformat.h)
find_path(AVUTIL_INCLUDE_DIR libavutil/avutil.h)
find_path(AVDEVICE_INCLUDE_DIR libavdevice/avdevice.h) #TODO dont be lazy, include swscale and avfilter
target_include_directories(Chernobot PRIVATE "/usr/local/include/SDL2" ${AVCODEC_INCLUDE_DIR} ${AVFORMAT_INCLUDE_DIR} ${AVUTIL_INCLUDE_DIR} ${AVDEVICE_INCLUDE_DIR})

find_library( AVUTIL_LIBRARY
  NAMES avutil
  HINTS
  $ENV{AVUTILDIR}
  PATH_SUFFIXES lib64 lib bin
  PATHS
  /usr/local
  /usr
  /sw
  /opt/local
  /opt/csw
  /opt
  /mingw
)
find_library( AVDEVICE_LIBRARY
  NAMES avdevice
  HINTS
  $ENV{AVUTILDIR}
  PATH_SUFFIXES lib64 lib bin
  PATHS
  /usr/local
  /usr
  /sw
  /opt/local
  /opt/csw
  /opt
  /mingw
)
find_library( AVCODEC_LIBRARY
  NAMES avcodec
  HINTS
  $ENV{AVUTILDIR}
  PATH_SUFFIXES lib64 lib bin
  PATHS
  /usr/local
  /usr
  /sw
  /opt/local
  /opt/csw
  /opt
  /mingw
)
find_library( AVFORMAT_LIBRARY
  NAMES avformat
  HINTS
  $ENV{AVUTILDIR}
  PATH_SUFFIXES lib64 lib bin
  PATHS
  /usr/local
  /usr
  /sw
  /opt/local
  /opt/csw
  /opt
  /mingw
)
find_library( AVFILTER_LIBRARY
  NAMES avfilter
  HINTS
  $ENV{AVUTILDIR}
  PATH_SUFFIXES lib64 lib bin
  PATHS
  /usr/local
  /usr
  /sw
  /opt/local
  /opt/csw
  /opt
  /mingw
)
find_library( SWSCALE_LIBRARY
  NAMES swscale
  HINTS
  $ENV{AVUTILDIR}
  PATH_SUFFIXES lib64 lib bin
  PATHS
  /usr/local
  /usr
  /sw
  /opt/local
  /opt/csw
  /opt
  /mingw
)


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
find_package(OpenCV REQUIRED)
target_link_libraries(Chernobot ${OpenCV_LIBS})
target_link_libraries(Chernobot ${AVUTIL_LIBRARY} ${AVDEVICE_LIBRARY} ${AVCODEC_LIBRARY} ${AVFORMAT_LIBRARY} ${AVFILTER_LIBRARY} ${SWSCALE_LIBRARY})
#target_link_libraries(Chernobot "-lavutil" "-lavdevice" "-lavcodec" "-lavformat" "-lavfilter" "-lswscale")
message( "AVUTIL_LIBRARY is ${AVDEVICE_LIBRARY}" )
#dependencies for serial lib - replace with rs232 also incomplete for linux and windows
if(APPLE)
	find_library(IOKIT_LIBRARY IOKit)
	find_library(FOUNDATION_LIBRARY Foundation)
	target_link_libraries(Chernobot ${FOUNDATION_LIBRARY} ${IOKIT_LIBRARY})
endif()


install(TARGETS Chernobot RUNTIME DESTINATION ${BIN_DIR})
