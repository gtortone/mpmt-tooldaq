
cmake_minimum_required (VERSION 2.6)

project (ToolDAQApplication)

set(CMAKE_CXX_FLAGS "-fPIC -O3  -std=c++11 -Wno-comment")

# set DEPENDENCIES_PATH to path containing libDAQInterface, ToolDAQFramework, ToolFrameworkCore libraries
set(DEPENDENCIES_PATH "/opt")

set(BOOST_LIBS boost_date_time boost_serialization boost_iostreams)
set(ZMQ_LIBS zmq)

set(DATAMODEL_LIB_PATH ${CMAKE_SOURCE_DIR}/lib)
set(DATAMODEL_LIBS "DataModel")

set(MYTOOLS_LIB_PATH ${CMAKE_SOURCE_DIR}/lib)
set(MYTOOLS_LIBS "MyTools")

set(TOOLFRAMEWORK_INC "${DEPENDENCIES_PATH}/ToolFrameworkCore/include")
set(TOOLFRAMEWORK_LIBS_PATH "${DEPENDENCIES_PATH}/ToolFrameworkCore/lib")
set(TOOLFRAMEWORK_LIBS DataModelBase Logging Store ToolChain)

set(TOOLDAQ_INC "${DEPENDENCIES_PATH}/ToolDAQFramework/include")
set(TOOLDAQ_LIBS_PATH "${DEPENDENCIES_PATH}/ToolDAQFramework/lib")
set(TOOLDAQ_LIBS ToolDAQChain DAQDataModelBase DAQLogging ServiceDiscovery DAQStore)

include_directories (
   ${DATAMODEL_INC} 
   ${MYTOOLS_INC} 
   ${TOOLFRAMEWORK_INC} 
   ${TOOLDAQ_INC} 
)

link_directories(
   ${DATAMODEL_LIB_PATH} 
   ${MYTOOLS_LIB_PATH} 
   ${TOOLFRAMEWORK_LIBS_PATH} 
   ${TOOLDAQ_LIBS_PATH} 
)

add_executable (main ${PROJECT_SOURCE_DIR}/src/main.cpp)

target_link_libraries (
   main
   MyTools
   DataModel
   ${TOOLDAQ_LIBS} 
   ${TOOLFRAMEWORK_LIBS} 
   ${DATAMODEL_LIBS} 
   ${MYTOOLS_LIBS} 
   ${ZMQ_LIBS} 
   ${BOOST_LIBS} 
)

MACRO(HEADER_DIRECTORIES return_list)
    FILE(GLOB_RECURSE new_list ${PROJECT_SOURCE_DIR}/src/*.h ${PROJECT_SOURCE_DIR}/DataModel/*.h ${PROJECT_SOURCE_DIR}/UserTools/*.h )
    FILE(COPY ${new_list} DESTINATION ${PROJECT_BINARY_DIR}/include)
    SET(dir_list "")
    FOREACH(file_path ${new_list})
        GET_FILENAME_COMPONENT(dir_path ${file_path} PATH)
        SET(dir_list ${dir_list} ${dir_path})
    ENDFOREACH()
    LIST(REMOVE_DUPLICATES dir_list)
    SET(${return_list} ${dir_list})
ENDMACRO()

FILE(COPY ${PROJECT_SOURCE_DIR}/configfiles DESTINATION ${PROJECT_BINARY_DIR}/)

HEADER_DIRECTORIES(header_list)
include_directories(${header_list})

set(CMAKE_LIBRARY_OUTPUT_DIRECTORY ${PROJECT_BINARY_DIR}/lib)

file(GLOB_RECURSE DATAMODEL_SRC RELATIVE ${CMAKE_SOURCE_DIR} "DataModel/*.cpp")
add_library(DataModel SHARED ${DATAMODEL_SRC})

file(GLOB_RECURSE MYTOOLS_SRC RELATIVE ${CMAKE_SOURCE_DIR} "UserTools/*.cpp")
add_library(MyTools SHARED ${MYTOOLS_SRC})


