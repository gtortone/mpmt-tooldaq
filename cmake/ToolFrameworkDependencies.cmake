function(find_zeromq)
    find_package(PkgConfig QUIET)
    if(PKG_CONFIG_FOUND)
        pkg_check_modules(PC_ZMQ QUIET libzmq)
    endif()

    find_path(ZMQ_INCLUDE_DIR
        NAMES zmq.h
        HINTS ${PC_ZMQ_INCLUDEDIR} ${PC_ZMQ_INCLUDE_DIRS}
              $ENV{ZMQ_ROOT}/include $ENV{ZEROMQ_ROOT}/include
              ${ZMQ_ROOT}/include
        PATHS /usr/include /usr/local/include /opt/include)

    find_library(ZMQ_LIBRARY
        NAMES zmq libzmq
        HINTS ${PC_ZMQ_LIBDIR} ${PC_ZMQ_LIBRARY_DIRS}
              $ENV{ZMQ_ROOT}/lib $ENV{ZEROMQ_ROOT}/lib
              ${ZMQ_ROOT}/lib
        PATHS /usr/lib /usr/local/lib /opt/lib)

    include(FindPackageHandleStandardArgs)
    find_package_handle_standard_args(ZeroMQ
        REQUIRED_VARS ZMQ_LIBRARY ZMQ_INCLUDE_DIR)

    if(ZeroMQ_FOUND AND NOT TARGET ToolFramework::ZeroMQ)
        add_library(ToolFramework::ZeroMQ UNKNOWN IMPORTED GLOBAL)
        set_target_properties(ToolFramework::ZeroMQ PROPERTIES
            IMPORTED_LOCATION             "${ZMQ_LIBRARY}"
            INTERFACE_INCLUDE_DIRECTORIES "${ZMQ_INCLUDE_DIR}")
    endif()

    # Esporta i risultati nel chiamante
    set(ZeroMQ_FOUND     ${ZeroMQ_FOUND}     PARENT_SCOPE)
    set(ZMQ_INCLUDE_DIRS ${ZMQ_INCLUDE_DIR}  PARENT_SCOPE)
    set(ZMQ_LIBRARIES    ${ZMQ_LIBRARY}      PARENT_SCOPE)
endfunction()

function(find_boost_dependencies)
    set(_components ${ARGN})
    if(NOT _components)
        set(_components date_time serialization iostreams)
    endif()

    # Preferisci BoostConfig.cmake quando disponibile (CMake >= 3.30)
    if(POLICY CMP0167)
        cmake_policy(SET CMP0167 NEW)
    endif()

    find_package(Boost REQUIRED COMPONENTS ${_components})

    set(Boost_FOUND        ${Boost_FOUND}        PARENT_SCOPE)
    set(Boost_INCLUDE_DIRS ${Boost_INCLUDE_DIRS} PARENT_SCOPE)
    set(Boost_LIBRARIES    ${Boost_LIBRARIES}    PARENT_SCOPE)
endfunction()
