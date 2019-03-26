# cmake -DCMAKE_BUILD_TYPE=Debug ..
# cmake -DCMAKE_BUILD_TYPE=Release ..
cmake_minimum_required(VERSION 3.0.2)
project(bento4 LANGUAGES CXX VERSION "0.0.0")

# Variables
set(SOURCE_ROOT ${CMAKE_CURRENT_SOURCE_DIR}/Source/C++)
set(SOURCE_CODECS ${SOURCE_ROOT}/Codecs)
set(SOURCE_CORE ${SOURCE_ROOT}/Core)
set(SOURCE_CRYPTO ${SOURCE_ROOT}/Crypto)
set(SOURCE_METADATA ${SOURCE_ROOT}/MetaData)
set(SOURCE_SYSTEM ${SOURCE_ROOT}/System)

# Compiler warning and optimization flags
set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -Wall")

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
file(GLOB AP4_HEADERS
  ${SOURCE_CORE}/*.h
  ${SOURCE_CODECS}/*.h
  ${SOURCE_CRYPTO}/*.h
  ${SOURCE_METADATA}/*.h
)

if(WIN32)
  set(AP4_SOURCES ${AP4_SOURCES} ${SOURCE_SYSTEM}/Win32/Ap4Win32Random.cpp)
else()
  set(AP4_SOURCES ${AP4_SOURCES} ${SOURCE_SYSTEM}/Posix/Ap4PosixRandom.cpp)
endif()

add_library(ap4 STATIC ${AP4_SOURCES})

# Includes
list(APPEND BENTO4_INCLUDE_DIRS
  ${SOURCE_CORE}
  ${SOURCE_CODECS}
  ${SOURCE_CRYPTO}
  ${SOURCE_METADATA}
)
target_include_directories(
  ap4
  PUBLIC
  "$<BUILD_INTERFACE:${BENTO4_INCLUDE_DIRS}>"
)

# Apps
file(GLOB BENTO4_APPS RELATIVE ${SOURCE_ROOT}/Apps ${SOURCE_ROOT}/Apps/*)
foreach(app ${BENTO4_APPS})
  string(TOLOWER ${app} binary_name)
  list(APPEND BENTO4_APPS_LOWERCASE ${binary_name})
  add_executable(${binary_name} ${SOURCE_ROOT}/Apps/${app}/${app}.cpp)
  target_link_libraries(${binary_name} ap4)
endforeach()

# Introduce variables:
# * CMAKE_INSTALL_LIBDIR
# * CMAKE_INSTALL_BINDIR
# * CMAKE_INSTALL_INCLUDEDIR
include(GNUInstallDirs)

set(GENERATED_DIR "${CMAKE_CURRENT_BINARY_DIR}/generated")
set(VERSION_CONFIG "${GENERATED_DIR}/${PROJECT_NAME}ConfigVersion.cmake")
set(PROJECT_CONFIG "${GENERATED_DIR}/${PROJECT_NAME}Config.cmake")

set(CONFIG_INSTALL_DIR "${CMAKE_INSTALL_LIBDIR}/cmake/${PROJECT_NAME}")

set(TARGETS_EXPORT_NAME "${PROJECT_NAME}Targets")
set(NAMESPACE "${PROJECT_NAME}::")

include(CMakePackageConfigHelpers)
write_basic_package_version_file(
  "${VERSION_CONFIG}" COMPATIBILITY ExactVersion
)

configure_package_config_file(
  "cmake/Config.cmake.in"
  "${PROJECT_CONFIG}"
  INSTALL_DESTINATION "${CONFIG_INSTALL_DIR}"
)

install(
  TARGETS ap4 ${BENTO4_APPS_LOWERCASE}
  EXPORT "${TARGETS_EXPORT_NAME}"
  INCLUDES DESTINATION "${CMAKE_INSTALL_INCLUDEDIR}"
  RUNTIME DESTINATION "${CMAKE_INSTALL_BINDIR}"
  LIBRARY DESTINATION "${CMAKE_INSTALL_LIBDIR}"
  ARCHIVE DESTINATION "${CMAKE_INSTALL_LIBDIR}"
)

install(
  FILES ${AP4_HEADERS}
  DESTINATION ${CMAKE_INSTALL_INCLUDEDIR}/${PROJECT_NAME}
)

install(
  FILES "${PROJECT_CONFIG}" "${VERSION_CONFIG}"
  DESTINATION "${CONFIG_INSTALL_DIR}"
)

install(
  EXPORT "${TARGETS_EXPORT_NAME}"
  NAMESPACE "${NAMESPACE}"
  DESTINATION "${CONFIG_INSTALL_DIR}"
)
