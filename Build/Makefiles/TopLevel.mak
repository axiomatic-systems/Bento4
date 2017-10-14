##########################################################################
#
#    top level make rules and variables
#
#    (c) 2002-2008 Axiomatic Systems, LLC
#    Author: Gilles Boccon-Gibod (bok@bok.net)
#
##########################################################################

##########################################################################
# exported variables
##########################################################################
BUILD_ROOT  = $(ROOT)/Build
SOURCE_ROOT = $(ROOT)/Source/C++

export BUILD_ROOT
export SOURCE_ROOT
export TARGET

export FILE_BYTE_STREAM_IMPLEMENTATION
export RANDOM_IMPLEMENTATION

export CC
export AUTODEP_CPP
export AUTODEP_STDOUT
export ARCHIVE
export COMPILE_CPP
export LINK_CPP
export MAKELIB
export MAKESHAREDLIB
export RANLIB
export STRIP
export DEBUG_CPP
export OPTIMIZE_CPP
export PROFILE_CPP
export DEFINES_CPP
export WARNINGS_CPP
export PIC_CPP
export INCLUDES_CPP
export LIBRARIES_CPP

##########################################################################
# modular targets
##########################################################################

# ------- Setup -------------
.PHONY: Setup
Setup:
	mkdir $(OUTPUT_DIR)
    
# ------- Apps -----------
ALL_APPS = mp4dump mp4info mp42aac mp42ts aac2mp4 mp4decrypt mp4encrypt mp4edit mp4extract mp4rtphintinfo mp4tag mp4dcfpackager mp4fragment mp4compact mp4split mp4mux avcinfo hevcinfo mp42hevc mp42hls
export ALL_APPS

##################################################################
# cleanup
##################################################################
TO_CLEAN += *.d *.o *.a *.exe $(ALL_APPS) SDK

##################################################################
# end targets
##################################################################
.PHONY: lib
lib:
	$(TITLE)
	@$(INVOKE_SUBMAKE) -f $(BUILD_ROOT)/Makefiles/Lib.mak

.PHONY: apps
apps: $(ALL_APPS)

 .PHONY: sdk
sdk: lib apps
	$(TITLE)
	@$(INVOKE_SUBMAKE) -f $(BUILD_ROOT)/Makefiles/SDK.mak
   
mp4dump: lib
	$(TITLE)
	@$(INVOKE_SUBMAKE) -f $(BUILD_ROOT)/Makefiles/Mp4Dump.mak

mp4info: lib
	$(TITLE)
	@$(INVOKE_SUBMAKE) -f $(BUILD_ROOT)/Makefiles/Mp4Info.mak

mp42aac: lib
	$(TITLE)
	@$(INVOKE_SUBMAKE) -f $(BUILD_ROOT)/Makefiles/Mp42Aac.mak

mp42ts: lib
	$(TITLE)
	@$(INVOKE_SUBMAKE) -f $(BUILD_ROOT)/Makefiles/Mp42Ts.mak

aac2mp4: lib
	$(TITLE)
	@$(INVOKE_SUBMAKE) -f $(BUILD_ROOT)/Makefiles/Aac2Mp4.mak

mp4decrypt: lib
	$(TITLE)
	@$(INVOKE_SUBMAKE) -f $(BUILD_ROOT)/Makefiles/Mp4Decrypt.mak

mp4encrypt: lib
	$(TITLE)
	@$(INVOKE_SUBMAKE) -f $(BUILD_ROOT)/Makefiles/Mp4Encrypt.mak

mp4edit: lib
	$(TITLE)
	@$(INVOKE_SUBMAKE) -f $(BUILD_ROOT)/Makefiles/Mp4Edit.mak

mp4extract: lib
	$(TITLE)
	@$(INVOKE_SUBMAKE) -f $(BUILD_ROOT)/Makefiles/Mp4Extract.mak

mp4rtphintinfo: lib
	$(TITLE)
	@$(INVOKE_SUBMAKE) -f $(BUILD_ROOT)/Makefiles/Mp4RtpHintInfo.mak

mp4tag: lib
	$(TITLE)
	@$(INVOKE_SUBMAKE) -f $(BUILD_ROOT)/Makefiles/Mp4Tag.mak

mp4dcfpackager: lib
	$(TITLE)
	@$(INVOKE_SUBMAKE) -f $(BUILD_ROOT)/Makefiles/Mp4DcfPackager.mak

mp4fragment: lib
	$(TITLE)
	@$(INVOKE_SUBMAKE) -f $(BUILD_ROOT)/Makefiles/Mp4Fragment.mak

mp4split: lib
	$(TITLE)
	@$(INVOKE_SUBMAKE) -f $(BUILD_ROOT)/Makefiles/Mp4Split.mak

mp4compact: lib
	$(TITLE)
	@$(INVOKE_SUBMAKE) -f $(BUILD_ROOT)/Makefiles/Mp4Compact.mak
	
mp4mux: lib
	$(TITLE)
	@$(INVOKE_SUBMAKE) -f $(BUILD_ROOT)/Makefiles/Mp4Mux.mak

avcinfo: lib
	$(TITLE)
	@$(INVOKE_SUBMAKE) -f $(BUILD_ROOT)/Makefiles/AvcInfo.mak

hevcinfo: lib
	$(TITLE)
	@$(INVOKE_SUBMAKE) -f $(BUILD_ROOT)/Makefiles/HevcInfo.mak

mp42hevc: lib
	$(TITLE)
	@$(INVOKE_SUBMAKE) -f $(BUILD_ROOT)/Makefiles/Mp42Hevc.mak

mp42hls: lib
	$(TITLE)
	@$(INVOKE_SUBMAKE) -f $(BUILD_ROOT)/Makefiles/Mp42Hls.mak

mp4iframeindex: lib
	$(TITLE)
	@$(INVOKE_SUBMAKE) -f $(BUILD_ROOT)/Makefiles/Mp4IframeIndex.mak

##################################################################
# includes
##################################################################
include $(BUILD_ROOT)/Makefiles/Rules.mak






