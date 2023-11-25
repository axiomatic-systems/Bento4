# cmake -DCMAKE_BUILD_TYPE=Debug ..
# cmake -DCMAKE_BUILD_TYPE=Release ..
if(MSVC)
  cmake_policy(SET CMP0091 NEW)
  cmake_minimum_required(VERSION 3.15)
else()
  cmake_minimum_required(VERSION 3.10)
endif()

function(get_bento4_version)
  file(STRINGS "Source/C++/Core/Ap4Version.h" _temp REGEX "AP4_VERSION_STRING")
  string(REGEX MATCH "\"([^\"]+)\"" _temp "${_temp}")
  set(BENTO4_VERSION "${CMAKE_MATCH_1}" PARENT_SCOPE)
endfunction()

get_bento4_version()
set(CMAKE_OSX_ARCHITECTURES "arm64;x86_64" CACHE STRING "")
project(bento4 VERSION "${BENTO4_VERSION}")

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

set(CMAKE_OSX_DEPLOYMENT_TARGET 10.12)

if(MSVC)
  add_definitions(-D_CRT_SECURE_NO_WARNINGS)
  set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} /GF /Gy /GR")
endif()

# AP4 Library
file(GLOB AP4_SOURCES
  ${SOURCE_CODECS}/*.cpp
  ${SOURCE_CORE}/*.cpp
  ${SOURCE_CRYPTO}/*.cpp
  ${SOURCE_METADATA}/*.cpp
  ${SOURCE_SYSTEM}/StdC/*.cpp
)

# Platform specifics
if(WIN32)
  set(AP4_SOURCES ${AP4_SOURCES} ${SOURCE_SYSTEM}/Win32/Ap4Win32Random.cpp)
else()
  set(AP4_SOURCES ${AP4_SOURCES} ${SOURCE_SYSTEM}/Posix/Ap4PosixRandom.cpp)
endif()

# Includes
set(AP4_INCLUDE_DIRS
  $<BUILD_INTERFACE:${SOURCE_CORE}>
  $<BUILD_INTERFACE:${SOURCE_CODECS}>
  $<BUILD_INTERFACE:${SOURCE_CRYPTO}>
  $<BUILD_INTERFACE:${SOURCE_METADATA}>
)
file(GLOB AP4_HEADERS
  ${SOURCE_CORE}/*.h
  ${SOURCE_CODECS}/*.h
  ${SOURCE_CRYPTO}/*.h
  ${SOURCE_METADATA}/*.h
)

add_library(ap4 STATIC ${AP4_SOURCES})
target_include_directories(ap4 PUBLIC
  ${AP4_INCLUDE_DIRS}
)

# Use the statically linked C runtime library
if(MSVC)
  set_property(TARGET ap4 PROPERTY MSVC_RUNTIME_LIBRARY "MultiThreaded$<$<CONFIG:Debug>:Debug>")
  target_compile_definitions(ap4 PRIVATE -D_LIB)
endif()

# Apps
option(BUILD_APPS "Build example applications" ON)
if(BUILD_APPS)
file(GLOB BENTO4_APPS RELATIVE ${SOURCE_ROOT}/Apps ${SOURCE_ROOT}/Apps/*)
foreach(app ${BENTO4_APPS})
  string(TOLOWER ${app} binary_name)
  list(APPEND BENTO4_APPS_LOWERCASE ${binary_name})
  add_executable(${binary_name} ${SOURCE_ROOT}/Apps/${app}/${app}.cpp)
  target_link_libraries(${binary_name} ap4)

  if(MSVC)
    set_property(TARGET ${binary_name} PROPERTY MSVC_RUNTIME_LIBRARY "MultiThreaded$<$<CONFIG:Debug>:Debug>")
    target_compile_definitions(${binary_name} PRIVATE -D_CONSOLE)
  endif()
endforeach()
endif(BUILD_APPS)

# Install
include(GNUInstallDirs)
set(config_install_dir "${CMAKE_INSTALL_LIBDIR}/cmake/${PROJECT_NAME}")
set(generated_dir "${CMAKE_CURRENT_BINARY_DIR}/generated")
set(version_config "${generated_dir}/${PROJECT_NAME}ConfigVersion.cmake")
set(project_config "${generated_dir}/${PROJECT_NAME}Config.cmake")
set(TARGETS_EXPORT_NAME "${PROJECT_NAME}Targets")
set(namespace "${PROJECT_NAME}::")

include(CMakePackageConfigHelpers)
write_basic_package_version_file(
    "${version_config}" COMPATIBILITY SameMajorVersion
)

configure_package_config_file(
    "Build/cmake/Config.cmake.in"
    "${project_config}"
    INSTALL_DESTINATION "${config_install_dir}"
)

install(
    TARGETS ap4 ${BENTO4_APPS_LOWERCASE}
    EXPORT "${TARGETS_EXPORT_NAME}"
    LIBRARY DESTINATION "${CMAKE_INSTALL_LIBDIR}"
    ARCHIVE DESTINATION "${CMAKE_INSTALL_LIBDIR}"
    RUNTIME DESTINATION "${CMAKE_INSTALL_BINDIR}"
    INCLUDES DESTINATION "${CMAKE_INSTALL_INCLUDEDIR}"
)

install(
    FILES ${AP4_HEADERS}
    DESTINATION "${CMAKE_INSTALL_INCLUDEDIR}/bento4"
)

install(
    FILES "${project_config}" "${version_config}"
    DESTINATION "${config_install_dir}"
)

install(
    EXPORT "${TARGETS_EXPORT_NAME}"
    NAMESPACE "${namespace}"
    DESTINATION "${config_install_dir}"
)
