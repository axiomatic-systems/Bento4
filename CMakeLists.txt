# cmake -DCMAKE_BUILD_TYPE=Debug ..
# cmake -DCMAKE_BUILD_TYPE=Release ..
cmake_minimum_required(VERSION 2.8)
project(bento4)

# Variables
set(SOURCE_ROOT ${CMAKE_CURRENT_SOURCE_DIR}/Source/C++)
set(SOURCE_CODECS ${SOURCE_ROOT}/Codecs)
set(SOURCE_CORE ${SOURCE_ROOT}/Core)
set(SOURCE_CRYPTO ${SOURCE_ROOT}/Crypto)
set(SOURCE_METADATA ${SOURCE_ROOT}/MetaData)
set(SOURCE_SYSTEM ${SOURCE_ROOT}/System)

# Compiler warning and optimization flags
if(CMAKE_C_COMPILER_ID MATCHES "GNU")
  set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -Wall -Wno-class-memaccess")
endif()

if (EMSCRIPTEN)
  set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -Wno-warn-absolute-paths")
endif()

# AP4 Library
file(GLOB AP4_SOURCES
  ${SOURCE_CODECS}/*.cpp
  ${SOURCE_CORE}/*.cpp
  ${SOURCE_CRYPTO}/*.cpp
  ${SOURCE_METADATA}/*.cpp
  ${SOURCE_SYSTEM}/StdC/*.cpp
)

if(WIN32)
  add_definitions(-D_CRT_SECURE_NO_WARNINGS)
  set(AP4_SOURCES ${AP4_SOURCES} ${SOURCE_SYSTEM}/Win32/Ap4Win32Random.cpp)
else()
  set(AP4_SOURCES ${AP4_SOURCES} ${SOURCE_SYSTEM}/Posix/Ap4PosixRandom.cpp)
endif()

# Includes
set(AP4_INCLUDE_DIRS
  ${SOURCE_CORE}
  ${SOURCE_CODECS}
  ${SOURCE_CRYPTO}
  ${SOURCE_METADATA}
)
include_directories(${AP4_INCLUDE_DIRS})

add_library(ap4 STATIC ${AP4_SOURCES})

# Apps
option(BUILD_APPS "Build example applications" ON)
if(BUILD_APPS)
file(GLOB BENTO4_APPS RELATIVE ${SOURCE_ROOT}/Apps ${SOURCE_ROOT}/Apps/*)
foreach(app ${BENTO4_APPS})
  string(TOLOWER ${app} binary_name)
  add_executable(${binary_name} ${SOURCE_ROOT}/Apps/${app}/${app}.cpp)
  target_link_libraries(${binary_name} ap4)
endforeach()
endif(BUILD_APPS)


file(GLOB AP4_INCLUDES
     ${SOURCE_CODECS}/*.h
     ${SOURCE_CORE}/*.h
     ${SOURCE_CRYPTO}/*.h
     ${SOURCE_METADATA}/*.h
)

install(TARGETS ap4
        EXPORT   ${PROJECT_NAME}Targets
        LIBRARY  DESTINATION lib
        ARCHIVE  DESTINATION lib
        RUNTIME  DESTINATION bin
        INCLUDES DESTINATION include)

install(EXPORT ${PROJECT_NAME}Targets
        FILE ${PROJECT_NAME}Targets.cmake
        NAMESPACE ${PROJECT_NAME}::
        DESTINATION cmake/${PROJECT_NAME})

install(FILES ${AP4_INCLUDES} DESTINATION include)

install(FILES 
        ${PROJECT_NAME}Config.cmake 
        DESTINATION cmake/${PROJECT_NAME})