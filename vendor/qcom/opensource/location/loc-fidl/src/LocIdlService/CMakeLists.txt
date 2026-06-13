cmake_minimum_required(VERSION 2.8)

set(PRJ_NAME LocIdlAPI)

set(CMAKE_VERBOSE_MAKEFILE on)
include(CheckCXXCompilerFlag)

OPTION(USE_FILE "Set to OFF to disable file logging" OFF )
message(STATUS "USE_FILE is set to value: ${USE_FILE}")

OPTION(USE_CONSOLE "Set to OFF to disable console logging" OFF )
message(STATUS "USE_CONSOLE is set to value: ${USE_CONSOLE}")

IF(USE_FILE)
  add_definitions(-DUSE_FILE)
ENDIF(USE_FILE)
IF(USE_CONSOLE)
  add_definitions(-DUSE_CONSOLE)
ENDIF(USE_CONSOLE)

add_definitions(-D__LINUX__)
add_definitions(-DOFF_TARGET)
add_definitions(-std=c++14)
add_definitions(-DBASE_PATH=/var/tmp/location)
add_definitions(-DUSE_ANDROID_LOGGING)

SET(MAX_LOG_LEVEL "DEBUG" CACHE STRING "maximum log level")
message(STATUS "MAX_LOG_LEVEL is set to value: ${MAX_LOG_LEVEL}")
add_definitions(-DCOMMONAPI_LOGLEVEL=COMMONAPI_LOGLEVEL_${MAX_LOG_LEVEL})

if (${CMAKE_SYSTEM_NAME} MATCHES "QNX")
    set (OS "QNX")
    set (NO_DEPRECATED_REGISTER "")
    set (Boost_USE_STATIC_LIBS ON)
endif (${CMAKE_SYSTEM_NAME} MATCHES "QNX")

if (MSVC)
# Visual C++ is not always sure whether he is really C++
set(CMAKE_C_FLAGS "${CMAKE_C_FLAGS} -D_CRT_SECURE_NO_WARNINGS /EHsc /wd\\\"4503\\\"")
set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -D_CRT_SECURE_NO_WARNINGS /wd\\\"4503\\\"")
else()
#set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -pthread -Wall -O0 -std=c++11 -D_GLIBCXX_USE_NANOSLEEP -DQNX")
set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -Wall -O0 -std=c++11 -D_GLIBCXX_USE_NANOSLEEP")
endif()

message(STATUS "Compiler options: ${CMAKE_CXX_FLAGS}")

if(NOT CMAKE_BUILD_TYPE)
   set(CMAKE_BUILD_TYPE "Debug" CACHE STRING
       "Choose the type of build, options are: Debug Release." FORCE)
endif(NOT CMAKE_BUILD_TYPE)
message(STATUS "Build type: ${CMAKE_BUILD_TYPE}")

OPTION(USE_INSTALLED_COMMONAPI "Set to OFF to use the local (build tree) version of CommonAPI" ON)
message(STATUS "USE_INSTALLED_COMMONAPI is set to value: ${USE_INSTALLED_COMMONAPI}")

if ("${USE_INSTALLED_COMMONAPI}" STREQUAL "ON")
    FIND_PACKAGE(CommonAPI 3.2.0 REQUIRED CONFIG NO_CMAKE_PACKAGE_REGISTRY)
    #FIND_PACKAGE(CommonAPI-DBus 3.2.0 REQUIRED CONFIG NO_CMAKE_PACKAGE_REGISTRY)
else()
    FIND_PACKAGE(CommonAPI 3.2.0 REQUIRED CONFIG NO_SYSTEM_ENVIRONMENT_PATH NO_CMAKE_SYSTEM_PATH)
    #FIND_PACKAGE(CommonAPI-DBus 3.2.0 REQUIRED CONFIG NO_SYSTEM_ENVIRONMENT_PATH NO_CMAKE_SYSTEM_PATH)
endif()

message(STATUS "CommonAPI_CONSIDERED_CONFIGS: ${CommonAPI_CONSIDERED_CONFIGS}")
message(STATUS "COMMONAPI_INCLUDE_DIRS: ${COMMONAPI_INCLUDE_DIRS}")
message(STATUS "CommonAPI-DBus_CONSIDERED_CONFIGS: ${CommonAPI-DBus_CONSIDERED_CONFIGS}")
message(STATUS "COMMONAPI_DBUS_INCLUDE_DIRS: ${COMMONAPI_DBUS_INCLUDE_DIRS}")

# CommonAPI
include(FindPkgConfig)
###############################################################################
# find DBus by using the 'pkg-config' tool
if (MSVC)
    #Not beautiful, but it works
    if (DBus_DIR)
        if (DBus_BUILD_DIR)
            set(DBus_INCLUDE_DIRS "${DBus_DIR};${DBus_BUILD_DIR};")
        else ()
            message (FATAL_ERROR "DBus_BUILD_DIR not set! Cannot continue.")
        endif ()
    else()
        message (FATAL_ERROR "DBus_DIR not set! Cannot continue.")
    endif ()
else()
#    pkg_check_modules(DBus REQUIRED dbus-1>=1.4)
endif()

SYSR_INCLUDE_DIR(qcom-gps-utils)
SYSR_INCLUDE_DIR(location-client-api)
SYSR_INCLUDE_DIR(open-avb)
SYSR_INCLUDE_DIR(location-integration-api)
SYSR_INCLUDE_DIR(loc-diag-iface-hdr)

check_cxx_compiler_flag("-DPOWER_DAEMON_MGR_ENABLED" FLAG_POWER_DAEMON_MGR_ENABLED)

if (FLAG_POWER_DAEMON_MGR_ENABLED)
    add_definitions(-DPOWER_DAEMON_MGR_ENABLED)
    SYSR_INCLUDE_DIR(powermanager)
else()
    message(STATUS "Flag -DPOWER_DAEMON_MGR_ENABLED is not supported.")
endif()

# SOME/IP
find_package (CommonAPI-SomeIP 3.2.0 REQUIRED)
find_package (vsomeip3 3.1.20 REQUIRED)

# Source Files
set(PRJ_SRC_PATH ../../src/LocIdlService)
set(PRJ_SRC_GEN_PATH ../../src-gen)
set(PRJ_SRC_GEN_COMMONAPI_PATH ${PRJ_SRC_GEN_PATH}/core/v0/com/qualcomm/qti/location)
set(PRJ_SRC_GEN_COMMONAPI_DBUS_PATH ${PRJ_SRC_GEN_PATH}/dbus/v0/com/qualcomm/qti/location)
set(PRJ_SRC_GEN_COMMONAPI_SOMEIP_PATH ${PRJ_SRC_GEN_PATH}/someip/v0/com/qualcomm/qti/location)

set(PRJ_NAME_SERVICE ${PRJ_NAME}Service)

# Application
FILE(GLOB PRJ_PROXY_GEN_SRCS ${PRJ_SRC_GEN_COMMONAPI_PATH}/*Proxy.cpp)
FILE(GLOB PRJ_STUB_GEN_SRCS ${PRJ_SRC_GEN_COMMONAPI_PATH}/*Stub*.cpp)
FILE(GLOB PRJ_STUB_IMPL_SRCS ${PRJ_SRC_COMMONAPI_PATH}/*Stub*.cpp)
set(PRJ_SERVICE_SRCS ${PRJ_SRC_PATH}/${PRJ_NAME_SERVICE}.cpp ${PRJ_SRC_PATH}/${PRJ_NAME}StubImpl.cpp ${PRJ_STUB_GEN_SRCS} ${PRJ_STUB_IMPL_SRCS} ${PRJ_SRC_PATH}/LocLcaIdlConverter.cpp ${PRJ_SRC_PATH}/LocIdlPowerEvtManager.cpp ${PRJ_SRC_PATH}/LocIdlServiceLog.cpp)

# Boost
#find_package( Boost 1.71 COMPONENTS system thread log REQUIRED )
include_directories( ${Boost_INCLUDE_DIR} )

# DBus library
FILE(GLOB PRJ_DBUS_LIB_SRCS ${PRJ_SRC_GEN_COMMONAPI_DBUS_PATH}/*cpp)

# SOME/IP library
FILE(GLOB PRJ_SOMEIP_LIB_SRCS ${PRJ_SRC_GEN_COMMONAPI_SOMEIP_PATH}/*cpp)

# Paths
OPTION(USE_INSTALLED_DBUS "Set to OFF to use the local (patched) version of dbus" ON)
message(STATUS "USE_INSTALLED_DBUS is set to value: ${USE_INSTALLED_DBUS}")

include_directories(
    ../../src-gen/core
    ../../src-gen/someip
    ${COMMONAPI_INCLUDE_DIRS}
    ${COMMONAPI_SOMEIP_INCLUDE_DIRS}
    ${VSOMEIP_INCLUDE_DIRS}
)

if ("${USE_INSTALLED_DBUS}" STREQUAL "ON")
link_directories(
    ${COMMONAPI_LIBDIR}
    ${COMMONAPI_SOMEIP_CMAKE_DIR}/build
    ${Boost_LIBRARY_DIR}
)
else()
link_directories(
    ${COMMONAPI_LIBDIR}
    ${COMMONAPI_SOMEIP_CMAKE_DIR}/build
    ${DBus_INCLUDE_DIRS}/dbus/.libs
    ${Boost_LIBRARY_DIR}
)
endif()

if (MSVC)
  set(LINK_LIBRARIES CommonAPI location_client_api gps_utils gptp location_integration_api log)
else()
  set(LINK_LIBRARIES -Wl,--as-needed CommonAPI location_client_api gps_utils gptp location_integration_api log)
endif()

# Build service
add_executable(${PRJ_NAME_SERVICE} ${PRJ_SERVICE_SRCS})
if (FLAG_POWER_DAEMON_MGR_ENABLED)
    target_link_libraries(${PRJ_NAME_SERVICE} ${LINK_LIBRARIES} powermanager)
else ()
    target_link_libraries(${PRJ_NAME_SERVICE} ${LINK_LIBRARIES})
endif()

# Build DBus library
#add_library (${PRJ_NAME}-dbus SHARED ${PRJ_DBUS_LIB_SRCS})
#target_link_libraries(${PRJ_NAME}-dbus CommonAPI-DBus)

# Build SOME/IP library
add_library (${PRJ_NAME}-someip SHARED ${PRJ_SOMEIP_LIB_SRCS})
target_link_libraries(${PRJ_NAME}-someip CommonAPI-SomeIP)
install (
#    TARGETS ${PRJ_NAME_CLIENT} ${PRJ_NAME_SERVICE}
    TARGETS ${PRJ_NAME_SERVICE}
    RUNTIME DESTINATION "${INSTALL_BIN_DIR}" COMPONENT bin)

install (
    TARGETS ${PRJ_NAME}-someip
    LIBRARY DESTINATION "${INSTALL_LIB_DIR}" COMPONENT shlib
)


set(EXAMPLE_CONFIG_FILES
    "vsomeip-location.json"
    "vsomeip-service.json"
    "commonapi4someip.ini"
    "vsomeip-local.json"
)

install (
    FILES ${EXAMPLE_CONFIG_FILES} DESTINATION /etc COMPONENT config
)

