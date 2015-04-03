cmake_minimum_required(VERSION 3.0)

# Variables
set(SOURCE_ROOT Source/C++)
set(SOURCE_CODECS ${SOURCE_ROOT}/Codecs)
set(SOURCE_CORE ${SOURCE_ROOT}/Core)
set(SOURCE_CRYPTO ${SOURCE_ROOT}/Crypto)
set(SOURCE_METADATA ${SOURCE_ROOT}/MetaData)
set(SOURCE_SYSTEM ${SOURCE_ROOT}/System)

# AP4 Library
file(GLOB AP4_SOURCES
  ${SOURCE_CODECS}/*.cpp
  ${SOURCE_CORE}/*.cpp
  ${SOURCE_CRYPTO}/*.cpp
  ${SOURCE_METADATA}/*.cpp
  ${SOURCE_SYSTEM}/StdC/*.cpp
)

if(WIN32)
  set(AP4_SOURCES ${AP4_SOURCES} ${SOURCE_SYSTEM}/Win32/Ap4Win32Random.cpp)
else()
  set(AP4_SOURCES ${AP4_SOURCES} ${SOURCE_SYSTEM}/Posix/Ap4PosixRandom.cpp)
endif()

add_library(ap4 STATIC ${AP4_SOURCES})

# Includes
include_directories(
  ${SOURCE_CORE}
  ${SOURCE_CODECS}
  ${SOURCE_CRYPTO}
  ${SOURCE_METADATA}
)

# Compiler warning and optimization flags
## cmake -DCMAKE_BUILD_TYPE=Debug ..
set(CMAKE_CXX_FLAGS_DEBUG "${CMAKE_CXX_FLAGS_DEBUG} -Wall -Wextra")
## cmake -DCMAKE_BUILD_TYPE=Release ..
set(CMAKE_CXX_FLAGS_RELEASE "${CMAKE_CXX_FLAGS_RELEASE} -Wall -Wextra -O3")

# Binaries
## aac2mp4
add_executable(aac2mp4 ${SOURCE_ROOT}/Apps/Aac2Mp4/Aac2Mp4.cpp)
target_link_libraries(aac2mp4 ap4)

## mp42aac
add_executable(mp42aac ${SOURCE_ROOT}/Apps/Mp42Aac/Mp42Aac.cpp)
target_link_libraries(mp42aac ap4)

## mp42ts
add_executable(mp42ts ${SOURCE_ROOT}/Apps/Mp42Ts/Mp42Ts.cpp)
target_link_libraries(mp42ts ap4)

## mp4compact
add_executable(mp4compact ${SOURCE_ROOT}/Apps/Mp4Compact/Mp4Compact.cpp)
target_link_libraries(mp4compact ap4)

## mp4dcfpackager
add_executable(mp4dcfpackager ${SOURCE_ROOT}/Apps/Mp4DcfPackager/Mp4DcfPackager.cpp)
target_link_libraries(mp4dcfpackager ap4)

## mp4decrypt
add_executable(mp4decrypt ${SOURCE_ROOT}/Apps/Mp4Decrypt/Mp4Decrypt.cpp)
target_link_libraries(mp4decrypt ap4)

## mp4dump
add_executable(mp4dump ${SOURCE_ROOT}/Apps/Mp4Dump/Mp4Dump.cpp)
target_link_libraries(mp4dump ap4)

## mp4edit
add_executable(mp4edit ${SOURCE_ROOT}/Apps/Mp4Edit/Mp4Edit.cpp)
target_link_libraries(mp4edit ap4)

## mp4encrypt
add_executable(mp4encrypt ${SOURCE_ROOT}/Apps/Mp4Encrypt/Mp4Encrypt.cpp)
target_link_libraries(mp4encrypt ap4)

## mp4extract
add_executable(mp4extract ${SOURCE_ROOT}/Apps/Mp4Extract/Mp4Extract.cpp)
target_link_libraries(mp4extract ap4)

## mp4fragment
add_executable(mp4fragment ${SOURCE_ROOT}/Apps/Mp4Fragment/Mp4Fragment.cpp)
target_link_libraries(mp4fragment ap4)

## mp4info
add_executable(mp4info ${SOURCE_ROOT}/Apps/Mp4Info/Mp4Info.cpp)
target_link_libraries(mp4info ap4)

## mp4mux
add_executable(mp4mux ${SOURCE_ROOT}/Apps/Mp4Mux/Mp4Mux.cpp)
target_link_libraries(mp4mux ap4)

## mp4rtphintinfo
add_executable(mp4rtphintinfo ${SOURCE_ROOT}/Apps/Mp4RtpHintInfo/Mp4RtpHintInfo.cpp)
target_link_libraries(mp4rtphintinfo ap4)

## mp4split
add_executable(mp4split ${SOURCE_ROOT}/Apps/Mp4Split/Mp4Split.cpp)
target_link_libraries(mp4split ap4)

## mp4tag
add_executable(mp4tag ${SOURCE_ROOT}/Apps/Mp4Tag/Mp4Tag.cpp)
target_link_libraries(mp4tag ap4)

