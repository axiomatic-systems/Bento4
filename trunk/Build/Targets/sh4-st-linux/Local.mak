#######################################################################
#
#   Makefile for st-sh4-linux
#
#######################################################################
TARGET = sh4-st-linux
GCC_CROSS_PREFIX=sh4-linux-

AP4_PLATFORM_BYTE_ORDER=AP4_PLATFORM_BYTE_ORDER_LITTLE_ENDIAN

include ../../any-gnu-gcc/Local.mak
DEFINES_CPP = $(DEFINES_CPP) -DSTMICROELECTRONICS
