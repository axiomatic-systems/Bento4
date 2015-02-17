#######################################################################
#
#   AP4 Makefile for any-gnu-gcc
#
#######################################################################
all: apps

#######################################################################
#    configuration variables
#######################################################################
#TARGET = any-gnu-gcc
ROOT   = ../../../..

#######################################################################
#    tools
#######################################################################
# how to make dependencies
AUTODEP_CPP = $(GCC_CROSS_PREFIX)gcc -MM

# how to make a library
MAKELIB = $(GCC_CROSS_PREFIX)ar rs

# how to optimize the layout of a library
RANLIB = $(GCC_CROSS_PREFIX)ranlib

# how to strip executables
STRIP = $(GCC_CROSS_PREFIX)strip

# how to compile source code
COMPILE_CPP  = $(GCC_CROSS_PREFIX)g++

# how to link object files
LINK_CPP = $(GCC_CROSS_PREFIX)g++ -L.

# optimization flags
OPTIMIZE_CPP = -O3 -ffunction-sections -fdata-sections

# debug flags
DEBUG_CPP = -g

# profiling flags
PROFILE_CPP = -pg

# position independent code flags
PIC_CPP = -fPIC

# compilation flags
ifneq ($(AP4_PLATFORM_BYTE_ORDER),)
DEFINES_CPP_BYTE_ORDER = -DAP4_PLATFORM_BYTE_ORDER=$(AP4_PLATFORM_BYTE_ORDER)
endif
DEFINES_CPP = -D_REENTRANT $(DEFINES_CPP_BYTE_ORDER)

# warning flags
WARNINGS_CPP = -Wall -Wshadow -Wpointer-arith -Wcast-qual 

# include directories
INCLUDES_CPP =

# libraries
LIBRARIES_CPP = 

#######################################################################
#    module selection
#######################################################################
FILE_BYTE_STREAM_IMPLEMENTATION = Ap4StdCFileByteStream
RANDOM_IMPLEMENTATION = Ap4PosixRandom

#######################################################################
#    includes
#######################################################################
include $(ROOT)/Build/Makefiles/TopLevel.mak
