##########################################################################
#
#    Bento4 Runtime Lib
#
#    (c) 2002-2009 Axiomatic Systems, LLC
#
##########################################################################
all: libAP4.a

CORE_SOURCES = 								\
    Ap4Results.cpp                          \
    Ap4Atom.cpp                             \
    Ap4AtomFactory.cpp                      \
    Ap4AtomSampleTable.cpp                  \
    Ap4AvccAtom.cpp                         \
    Ap4ByteStream.cpp                       \
    Ap4Co64Atom.cpp                         \
    Ap4ContainerAtom.cpp                    \
    Ap4CttsAtom.cpp                         \
    Ap4DataBuffer.cpp                       \
    Ap4Debug.cpp                            \
    Ap4DecoderConfigDescriptor.cpp          \
    Ap4DecoderSpecificInfoDescriptor.cpp    \
    Ap4Descriptor.cpp                       \
    Ap4DescriptorFactory.cpp                \
    Ap4DrefAtom.cpp                         \
    Ap4EsDescriptor.cpp                     \
    Ap4EsdsAtom.cpp                         \
    Ap4File.cpp                             \
    Ap4FileWriter.cpp                       \
    Ap4FileCopier.cpp                       \
    Ap4FrmaAtom.cpp                         \
    Ap4FtypAtom.cpp                         \
    Ap4HdlrAtom.cpp                         \
    Ap4HintTrackReader.cpp                  \
    Ap4HmhdAtom.cpp                         \
    Ap4IkmsAtom.cpp                         \
    Ap4IproAtom.cpp                         \
    Ap4IsfmAtom.cpp                         \
    Ap4IsltAtom.cpp                         \
    Ap4IsmaCryp.cpp                         \
    Ap4MdhdAtom.cpp                         \
    Ap4MoovAtom.cpp                         \
    Ap4Movie.cpp                            \
    Ap4MvhdAtom.cpp                         \
    Ap4NmhdAtom.cpp                         \
    Ap4SthdAtom.cpp                         \
    Ap4OdafAtom.cpp                         \
    Ap4OddaAtom.cpp                         \
    Ap4OdheAtom.cpp                         \
    Ap4OhdrAtom.cpp                         \
    Ap4OmaDcf.cpp                           \
    Ap4Processor.cpp                        \
    Ap4Protection.cpp                       \
    Ap4RtpAtom.cpp                          \
    Ap4RtpHint.cpp                          \
    Ap4Sample.cpp                           \
    Ap4SampleDescription.cpp                \
    Ap4SampleEntry.cpp                      \
    Ap4SampleTable.cpp                      \
    Ap4SchmAtom.cpp                         \
    Ap4SdpAtom.cpp                          \
    Ap4SLConfigDescriptor.cpp               \
    Ap4SmhdAtom.cpp                         \
    Ap4StcoAtom.cpp                         \
    Ap4String.cpp                           \
    Ap4StscAtom.cpp                         \
    Ap4StsdAtom.cpp                         \
    Ap4StssAtom.cpp                         \
    Ap4StszAtom.cpp                         \
    Ap4Stz2Atom.cpp							\
    Ap4SttsAtom.cpp                         \
    Ap4SyntheticSampleTable.cpp             \
    Ap4TimsAtom.cpp                         \
    Ap4TkhdAtom.cpp                         \
    Ap4Track.cpp                            \
    Ap4TrakAtom.cpp                         \
    Ap4TrefTypeAtom.cpp                     \
    Ap4UrlAtom.cpp                          \
    Ap4Utils.cpp                            \
    Ap4VmhdAtom.cpp                         \
    Ap4ElstAtom.cpp                         \
    Ap4Expandable.cpp                       \
    Ap4GrpiAtom.cpp                         \
    Ap4Ipmp.cpp                             \
    Ap4IodsAtom.cpp                         \
    Ap4ObjectDescriptor.cpp                 \
    Ap4Command.cpp                          \
    Ap4CommandFactory.cpp                   \
    Ap4UuidAtom.cpp                         \
    Ap4Marlin.cpp                           \
    Ap4TrunAtom.cpp                         \
    Ap4TfhdAtom.cpp                         \
    Ap4MfhdAtom.cpp                         \
    Ap48bdlAtom.cpp                         \
    Ap4MehdAtom.cpp                         \
    Ap4TrexAtom.cpp                         \
    Ap4LinearReader.cpp			            \
    Ap4Mpeg2Ts.cpp                          \
    Ap4Hmac.cpp                             \
    Ap4KeyWrap.cpp 							\
    Ap4MovieFragment.cpp                    \
    Ap4FragmentSampleTable.cpp              \
    Ap4Piff.cpp                             \
    Ap4TfraAtom.cpp                         \
    Ap4MfroAtom.cpp							\
    Ap4TfdtAtom.cpp							\
    Ap4CommonEncryption.cpp					\
    Ap4SaioAtom.cpp							\
    Ap4SaizAtom.cpp							\
    Ap4SencAtom.cpp							\
    Ap4TencAtom.cpp							\
    Ap4TfdtAtom.cpp                         \
    Ap4PdinAtom.cpp                         \
    Ap4AinfAtom.cpp                         \
    Ap4BlocAtom.cpp                         \
    Ap4PsshAtom.cpp			                \
    Ap4Dec3Atom.cpp                         \
    Ap4SidxAtom.cpp                         \
    Ap4HvccAtom.cpp                         \
    Ap4DvccAtom.cpp                         \
    Ap4SgpdAtom.cpp                         \
    Ap4SbgpAtom.cpp                         \
    Ap4NalParser.cpp                        \
    Ap4AvcParser.cpp                        \
    Ap4HevcParser.cpp                       \
    Ap4SegmentBuilder.cpp                   \


CORE_OBJECTS=$(CORE_SOURCES:.cpp=.o)

CRYPTO_SOURCES = Ap4StreamCipher.cpp Ap4AesBlockCipher.cpp
CRYPTO_OBJECTS = $(CRYPTO_SOURCES:.cpp=.o)

METADATA_SOURCES = Ap4MetaData.cpp
METADATA_OBJECTS = $(METADATA_SOURCES:.cpp=.o)

SYSTEM_SOURCES = $(FILE_BYTE_STREAM_IMPLEMENTATION).cpp $(RANDOM_IMPLEMENTATION).cpp
SYSTEM_OBJECTS = $(SYSTEM_SOURCES:.cpp=.o)

CODECS_SOURCES = Ap4AdtsParser.cpp Ap4BitStream.cpp Ap4Mp4AudioInfo.cpp
CODECS_OBJECTS = $(CODECS_SOURCES:.cpp=.o)

CAPI_SOURCES = Bento4C.cpp
CAPI_OBJECTS = $(CAPI_SOURCES:.cpp=.o)

TARGET_SOURCES = $(CORE_SOURCES) $(SYSTEM_SOURCES) $(METADATA_SOURCES) $(CRYPTO_SOURCES) $(CODECS_SOURCES) $(CAPI_SOURCES)

##########################################################################
# search path
##########################################################################
VPATH += $(SOURCE_ROOT)/Core
VPATH += $(SOURCE_ROOT)/Crypto
VPATH += $(SOURCE_ROOT)/System/StdC
VPATH += $(SOURCE_ROOT)/System/Posix
VPATH += $(SOURCE_ROOT)/Codecs
VPATH += $(SOURCE_ROOT)/MetaData
VPATH += $(SOURCE_ROOT)/CApi

##########################################################################
# includes
##########################################################################
include $(BUILD_ROOT)/Makefiles/Lib.exp

##########################################################################
# includes
##########################################################################
include $(BUILD_ROOT)/Makefiles/Rules.mak

##########################################################################
# rules
##########################################################################
libAP4.a: $(CORE_OBJECTS) $(SYSTEM_OBJECTS) $(CRYPTO_OBJECTS) $(METADATA_OBJECTS) $(CODECS_OBJECTS) $(CAPI_OBJECTS)
