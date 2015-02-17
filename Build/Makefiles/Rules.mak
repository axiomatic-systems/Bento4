##########################################################################
#
#    common make rules and variables
#
#    (c) 2002-2008 Axiomatic Systems, LLC
#    Author: Gilles Boccon-Gibod (bok@bok.net)
#
##########################################################################

##########################################################################
# build configurations
##########################################################################
#VPATH += $(AP4_BUILD_CONFIG)

COMPILE_CPP_OPTIONS = $(PIC_CPP) $(WARNINGS_CPP) 

ifeq ($(AP4_BUILD_CONFIG),Profile)
COMPILE_CPP_OPTIONS += $(PROFILE_CPP)
endif
ifeq ($(AP4_BUILD_CONFIG),Debug)
COMPILE_CPP_OPTIONS += $(DEBUG_CPP)
else
COMPILE_CPP_OPTIONS += $(OPTIMIZE_CPP)
endif

##########################################################################
# default rules
##########################################################################
%.d: %.cpp
	$(AUTODEP_CPP) $(DEFINES_CPP) $(INCLUDES_CPP) $< -o $@

%.o: %.cpp
	$(COMPILE_CPP) $(COMPILE_CPP_OPTIONS) $($@_LOCAL_DEFINES_CPP) $(DEFINES_CPP) $(INCLUDES_CPP) -c $< -o $@

%.a:
	$(MAKELIB) $@ $^
	$(RANLIB) $@

.PHONY: clean
clean:
	@rm -rf $(TO_CLEAN)

TITLE = @echo ============ making $@ =============
INVOKE_SUBMAKE = $(MAKE) --no-print-directory

##########################################################################
# variables
##########################################################################
LINK                 = $(LINK_CPP)
LINK_LIBRARIES      += $(foreach lib,$(TARGET_LIBRARIES),-l$(lib))
TARGET_LIBRARY_FILES = $(foreach lib,$(TARGET_LIBRARIES),lib$(lib).a)
TARGET_OBJECTS       = $(TARGET_SOURCES:.cpp=.o)

##########################################################################
# auto dependencies
##########################################################################
TARGET_DEPENDENCIES := $(TARGET_SOURCES:.cpp=.d)

ifneq ($(TARGET_DEPENDENCIES),)
include $(TARGET_DEPENDENCIES)
endif

##########################################################################
# includes
##########################################################################
ifneq ($(LOCAL_RULES),)
include $(LOCAL_RULES)
endif
