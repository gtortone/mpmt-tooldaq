cmake_minimum_required(VERSION 3.20)

project(mpmt-tooldaq LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 11)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

file(GLOB_RECURSE SOURCES
    "${CMAKE_SOURCE_DIR}/src/*.cpp"
)

list(APPEND CMAKE_MODULE_PATH "${CMAKE_CURRENT_SOURCE_DIR}/cmake")
include(ToolFrameworkDependencies)

find_zeromq()
find_boost_dependencies(date_time serialization iostreams)
find_package(Threads REQUIRED)

include_directories(${Boost_INCLUDE_DIRS})

#
# change PATHS here if libToolDAQ.so is in not-default path
#
find_library(TOOLDAQ_SHARED
    NAMES ToolDAQ
    PATHS /opt/tooldaq/lib
)

#
# change PATHS here if libToolDAQ.a is in not-default path
#
find_library(TOOLDAQ_STATIC
    NAMES libToolDAQ.a ToolDAQ
    PATHS /opt/tooldaq/lib
)

add_library(tooldaq_common INTERFACE)
target_link_libraries(tooldaq_common INTERFACE Threads::Threads)
target_link_libraries(tooldaq_common INTERFACE ToolFramework::ZeroMQ)
target_link_libraries(tooldaq_common INTERFACE Boost::date_time Boost::serialization Boost::iostreams)
target_link_libraries(tooldaq_common INTERFACE zstd)

#
# executable with shared object
#
add_executable(${PROJECT_NAME} ${SOURCES})
target_link_libraries(${PROJECT_NAME} PRIVATE ${TOOLDAQ_SHARED})
target_compile_options(${PROJECT_NAME} PRIVATE 
	-Wall -Wextra -O3 
	-Wno-unused-result -Wno-deprecated-declarations 
	-Wno-unused-parameter -Wno-sign-compare)
target_include_directories(${PROJECT_NAME} PRIVATE ${CMAKE_CURRENT_SOURCE_DIR}/include)
target_link_libraries(${PROJECT_NAME} PRIVATE tooldaq_common)
#

#
# executable with static object
#
add_executable(${PROJECT_NAME}.static ${SOURCES})
target_link_libraries(${PROJECT_NAME}.static PRIVATE ${TOOLDAQ_STATIC})
target_compile_options(${PROJECT_NAME}.static PRIVATE 
	-Wall -Wextra -O3 
	-Wno-unused-result -Wno-deprecated-declarations 
	-Wno-unused-parameter -Wno-sign-compare)
target_include_directories(${PROJECT_NAME}.static PRIVATE ${CMAKE_CURRENT_SOURCE_DIR}/include)
target_link_libraries(${PROJECT_NAME}.static PRIVATE tooldaq_common)
#

file(COPY ${CMAKE_CURRENT_SOURCE_DIR}/configfiles DESTINATION ${CMAKE_BINARY_DIR}/)
