cmake_minimum_required(VERSION 3.13)

project(main VERSION 1.0)

set(CMAKE_CXX_STANDARD 11)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_FLAGS "-fPIC -Wpedantic -Wno-deprecated-declarations -O3")

# to add a new Tool define source path here
file(GLOB_RECURSE SRC_FILES 
   ${CMAKE_CURRENT_SOURCE_DIR}/src/*.cpp 
)
add_executable(${PROJECT_NAME} ${SRC_FILES})

target_include_directories(${PROJECT_NAME} PUBLIC ${CMAKE_CURRENT_SOURCE_DIR}/include)

find_package(ZeroMQ REQUIRED)
target_link_libraries(${PROJECT_NAME} PUBLIC zmq)
