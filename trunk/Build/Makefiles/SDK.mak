##########################################################################
#
#    SDK Makefile
#
#    (c) 2002-2008 Axiomatic Systems, LLC
#
##########################################################################
all: sdk

##########################################################################
# includes
##########################################################################
include $(BUILD_ROOT)/Makefiles/Lib.exp

##########################################################################
# variables
##########################################################################
SDK_LIBRARIES := libAP4.a
SDK_HEADERS := $(SOURCE_ROOT)/Core/*.h $(SOURCE_ROOT)/MetaData/*.h $(SOURCE_ROOT)/Crypto/*.h $(SOURCE_ROOT)/Codecs/*.h
SDK_BINARIES = $(ALL_APPS)

##########################################################################
# rules
##########################################################################
.PHONY: sdk-dirs
sdk-dirs:
	@rm -rf SDK
	@mkdir SDK
	@mkdir SDK/lib
	@mkdir SDK/include
	@mkdir SDK/bin

sdk: sdk-dirs $(SDK_LIBRARY) $(SDK_HEADERS)
	@cp $(SDK_HEADERS) SDK/include
	@cp $(SDK_BINARIES) SDK/bin
	@cp $(SDK_LIBRARIES) SDK/lib
	@$(STRIP) SDK/bin/*

##########################################################################
# includes
##########################################################################
include $(BUILD_ROOT)/Makefiles/Rules.mak


