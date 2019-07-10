/*****************************************************************
|
|    AP4 - sample entries
|
|    Copyright 2002-2008 Axiomatic Systems, LLC
|
|
|    This file is part of Bento4/AP4 (MP4 Atom Processing Library).
|
|    Unless you have obtained Bento4 under a difference license,
|    this version of Bento4 is Bento4|GPL.
|    Bento4|GPL is free software; you can redistribute it and/or modify
|    it under the terms of the GNU General Public License as published by
|    the Free Software Foundation; either version 2, or (at your option)
|    any later version.
|
|    Bento4|GPL is distributed in the hope that it will be useful,
|    but WITHOUT ANY WARRANTY; without even the implied warranty of
|    MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
|    GNU General Public License for more details.
|
|    You should have received a copy of the GNU General Public License
|    along with Bento4|GPL; see the file COPYING.  If not, write to the
|    Free Software Foundation, 59 Temple Place - Suite 330, Boston, MA
|    02111-1307, USA.
|
 ****************************************************************/

/*----------------------------------------------------------------------
|   includes
+---------------------------------------------------------------------*/
#include "Ap4SampleEntry.h"
#include "Ap4Utils.h"
#include "Ap4AtomFactory.h"
#include "Ap4TimsAtom.h"
#include "Ap4SampleDescription.h"
#include "Ap4AvccAtom.h"

/*----------------------------------------------------------------------
|   dynamic cast support
+---------------------------------------------------------------------*/
AP4_DEFINE_DYNAMIC_CAST_ANCHOR(AP4_SampleEntry)

/*----------------------------------------------------------------------
|   AP4_SampleEntry::AP4_SampleEntry
+---------------------------------------------------------------------*/
AP4_SampleEntry::AP4_SampleEntry(AP4_Atom::Type format, const AP4_AtomParent* details) :
    AP4_ContainerAtom(format),
    m_DataReferenceIndex(1)
{
    m_Reserved1[0] = 0;
    m_Reserved1[1] = 0;
    m_Reserved1[2] = 0;
    m_Reserved1[3] = 0;
    m_Reserved1[4] = 0;
    m_Reserved1[5] = 0;
    m_Size32 += 8;
    
    if (details) {
        details->CopyChildren(*this);
    }
}

/*----------------------------------------------------------------------
|   AP4_SampleEntry::AP4_SampleEntry
+---------------------------------------------------------------------*/
AP4_SampleEntry::AP4_SampleEntry(AP4_Atom::Type format,
                                 AP4_Size       size) :
    AP4_ContainerAtom(format, (AP4_UI64)size, false),
    m_DataReferenceIndex(1)
{
    m_Reserved1[0] = 0;
    m_Reserved1[1] = 0;
    m_Reserved1[2] = 0;
    m_Reserved1[3] = 0;
    m_Reserved1[4] = 0;
    m_Reserved1[5] = 0;
}

/*----------------------------------------------------------------------
|   AP4_SampleEntry::AP4_SampleEntry
+---------------------------------------------------------------------*/
AP4_SampleEntry::AP4_SampleEntry(AP4_Atom::Type   format,
                                 AP4_Size         size,
                                 AP4_ByteStream&  stream,
                                 AP4_AtomFactory& atom_factory) :
    AP4_ContainerAtom(format, (AP4_UI64)size, false)
{
    Read(stream, atom_factory);
}

/*----------------------------------------------------------------------
|   AP4_SampleEntry::Clone
+---------------------------------------------------------------------*/
AP4_Atom*
AP4_SampleEntry::Clone()
{
    return this->AP4_Atom::Clone();
}

/*----------------------------------------------------------------------
|   AP4_SampleEntry::Read
+---------------------------------------------------------------------*/
void
AP4_SampleEntry::Read(AP4_ByteStream& stream, AP4_AtomFactory& atom_factory)
{
    // read the fields before the children atoms
    ReadFields(stream);

    // read children atoms (ex: esds and maybe others)
    // NOTE: not all sample entries have children atoms
    AP4_Size payload_size = (AP4_Size)(GetSize()-GetHeaderSize());
    AP4_Size fields_size = GetFieldsSize();
    if (payload_size > fields_size) {
        ReadChildren(atom_factory, stream, payload_size-fields_size);
    }
}

/*----------------------------------------------------------------------
|   AP4_SampleEntry::GetFieldsSize
+---------------------------------------------------------------------*/
AP4_Size
AP4_SampleEntry::GetFieldsSize()
{
    return 8;
}

/*----------------------------------------------------------------------
|   AP4_SampleEntry::ReadFields
+---------------------------------------------------------------------*/
AP4_Result
AP4_SampleEntry::ReadFields(AP4_ByteStream& stream)
{
    stream.Read(m_Reserved1, sizeof(m_Reserved1));
    stream.ReadUI16(m_DataReferenceIndex);

    return AP4_SUCCESS;
}

/*----------------------------------------------------------------------
|   AP4_SampleEntry::WriteFields
+---------------------------------------------------------------------*/
AP4_Result
AP4_SampleEntry::WriteFields(AP4_ByteStream& stream)
{
    AP4_Result result;
    
    // reserved1
    result = stream.Write(m_Reserved1, sizeof(m_Reserved1));
    if (AP4_FAILED(result)) return result;

    // data reference index
    result = stream.WriteUI16(m_DataReferenceIndex);
    if (AP4_FAILED(result)) return result;
    
    return result;
}

/*----------------------------------------------------------------------
|   AP4_SampleEntry::Write
+---------------------------------------------------------------------*/
AP4_Result
AP4_SampleEntry::Write(AP4_ByteStream& stream)
{
    AP4_Result result;

    // write the header
    result = WriteHeader(stream);
    if (AP4_FAILED(result)) return result;

    // write the fields
    result = WriteFields(stream);
    if (AP4_FAILED(result)) return result;

    // write the children atoms
    return m_Children.Apply(AP4_AtomListWriter(stream));
}

/*----------------------------------------------------------------------
|   AP4_SampleEntry::InspectFields
+---------------------------------------------------------------------*/
AP4_Result
AP4_SampleEntry::InspectFields(AP4_AtomInspector& inspector)
{
    inspector.AddField("data_reference_index", m_DataReferenceIndex);

    return AP4_SUCCESS;
}

/*----------------------------------------------------------------------
|   AP4_SampleEntry::Inspect
+---------------------------------------------------------------------*/
AP4_Result
AP4_SampleEntry::Inspect(AP4_AtomInspector& inspector)
{
    // inspect the header
    InspectHeader(inspector);

    // inspect the fields
    InspectFields(inspector);

    // inspect children
    m_Children.Apply(AP4_AtomListInspector(inspector));

    // finish
    inspector.EndAtom();

    return AP4_SUCCESS;
}

/*----------------------------------------------------------------------
|   AP4_SampleEntry::OnChildChanged
+---------------------------------------------------------------------*/
void
AP4_SampleEntry::OnChildChanged(AP4_Atom*)
{
    // recompute our size
    AP4_UI64 size = GetHeaderSize()+GetFieldsSize();
    m_Children.Apply(AP4_AtomSizeAdder(size));
    m_Size32 = (AP4_UI32)size;

    // update our parent
    if (m_Parent) m_Parent->OnChildChanged(this);
}

/*----------------------------------------------------------------------
|   AP4_SampleEntry::ToSampleDescription
+---------------------------------------------------------------------*/
AP4_SampleDescription*
AP4_SampleEntry::ToSampleDescription()
{
    return new AP4_SampleDescription(AP4_SampleDescription::TYPE_UNKNOWN, m_Type, this);
}

/*----------------------------------------------------------------------
|   AP4_UnknownSampleEntry::AP4_UnknownSampleEntry
+---------------------------------------------------------------------*/
AP4_UnknownSampleEntry::AP4_UnknownSampleEntry(AP4_Atom::Type  type, 
                                               AP4_Size        size, 
                                               AP4_ByteStream& stream) :
    AP4_SampleEntry(type, size)
{
    if (size > AP4_ATOM_HEADER_SIZE+AP4_SampleEntry::GetFieldsSize()) {
        m_Payload.SetDataSize(size-(AP4_ATOM_HEADER_SIZE+AP4_SampleEntry::GetFieldsSize()));
        ReadFields(stream);
    }
}

/*----------------------------------------------------------------------
|   AP4_UnknownSampleEntry::AP4_UnknownSampleEntry
+---------------------------------------------------------------------*/
AP4_UnknownSampleEntry::AP4_UnknownSampleEntry(AP4_Atom::Type  type, 
                                               AP4_DataBuffer& payload) :
    AP4_SampleEntry(type),
    m_Payload(payload)
{
    m_Size32 += payload.GetDataSize();
}

/*----------------------------------------------------------------------
|   AP4_UnknownSampleEntry::Clone
+---------------------------------------------------------------------*/
AP4_Atom*
AP4_UnknownSampleEntry::Clone()
{
    return new AP4_UnknownSampleEntry(m_Type, m_Payload);
}

/*----------------------------------------------------------------------
|   AP4_UnknownSampleEntry::ToSampleDescription
+---------------------------------------------------------------------*/
AP4_SampleDescription* 
AP4_UnknownSampleEntry::ToSampleDescription()
{
    return new AP4_UnknownSampleDescription(this);
}

/*----------------------------------------------------------------------
|   AP4_UnknownSampleEntry::GetFieldsSize
+---------------------------------------------------------------------*/
AP4_Size   
AP4_UnknownSampleEntry::GetFieldsSize()
{
    return AP4_SampleEntry::GetFieldsSize()+m_Payload.GetDataSize();
}

/*----------------------------------------------------------------------
|   AP4_UnknownSampleEntry::ReadFields
+---------------------------------------------------------------------*/
AP4_Result
AP4_UnknownSampleEntry::ReadFields(AP4_ByteStream& stream)
{
    // sample entry
    AP4_Result result = AP4_SampleEntry::ReadFields(stream);
    if (AP4_FAILED(result)) return result;
    
    // read the payload
    return stream.Read(m_Payload.UseData(), m_Payload.GetDataSize());
}

/*----------------------------------------------------------------------
|   AP4_UnknownSampleEntry::WriteFields
+---------------------------------------------------------------------*/
AP4_Result
AP4_UnknownSampleEntry::WriteFields(AP4_ByteStream& stream)
{
    AP4_Result result;
    
    // write the fields of the base class
    result = AP4_SampleEntry::WriteFields(stream);
    if (AP4_FAILED(result)) return result;
    
    // write the payload
    return stream.Write(m_Payload.GetData(), m_Payload.GetDataSize());
}

/*----------------------------------------------------------------------
|   AP4_MpegSystemSampleEntry::AP4_MpegSystemSampleEntry
+---------------------------------------------------------------------*/
AP4_MpegSystemSampleEntry::AP4_MpegSystemSampleEntry(
    AP4_UI32          type,
    AP4_EsDescriptor* descriptor) :
    AP4_SampleEntry(type)
{
    if (descriptor) AddChild(new AP4_EsdsAtom(descriptor));
}

/*----------------------------------------------------------------------
|   AP4_MpegSystemSampleEntry::AP4_MpegSystemSampleEntry
+---------------------------------------------------------------------*/
AP4_MpegSystemSampleEntry::AP4_MpegSystemSampleEntry(
    AP4_UI32         type,
    AP4_Size         size,
    AP4_ByteStream&  stream,
    AP4_AtomFactory& atom_factory) :
    AP4_SampleEntry(type, size, stream, atom_factory)
{
}

/*----------------------------------------------------------------------
|   AP4_MpegSystemSampleEntry::ToSampleDescription
+---------------------------------------------------------------------*/
AP4_SampleDescription*
AP4_MpegSystemSampleEntry::ToSampleDescription()
{
    return new AP4_MpegSystemSampleDescription(
        AP4_DYNAMIC_CAST(AP4_EsdsAtom, GetChild(AP4_ATOM_TYPE_ESDS)));
}

/*----------------------------------------------------------------------
|   AP4_Mp4sSampleEntry::AP4_Mp4sSampleEntry
+---------------------------------------------------------------------*/
AP4_Mp4sSampleEntry::AP4_Mp4sSampleEntry(AP4_EsDescriptor* descriptor) :
    AP4_MpegSystemSampleEntry(AP4_ATOM_TYPE_MP4S, descriptor)
{
}

/*----------------------------------------------------------------------
|   AP4_Mp4sSampleEntry::AP4_Mp4sSampleEntry
+---------------------------------------------------------------------*/
AP4_Mp4sSampleEntry::AP4_Mp4sSampleEntry(AP4_Size         size,
                                         AP4_ByteStream&  stream,
                                         AP4_AtomFactory& atom_factory) :
    AP4_MpegSystemSampleEntry(AP4_ATOM_TYPE_MP4S, size, stream, atom_factory)
{
}

/*----------------------------------------------------------------------
|   AP4_Mp4sSampleEntry::ToSampleDescription
+---------------------------------------------------------------------*/
AP4_SampleDescription*
AP4_Mp4sSampleEntry::ToSampleDescription()
{
    // create a sample description
    return new AP4_MpegSystemSampleDescription(
        AP4_DYNAMIC_CAST(AP4_EsdsAtom, GetChild(AP4_ATOM_TYPE_ESDS)));
}

/*----------------------------------------------------------------------
|   AP4_AudioSampleEntry::AP4_AudioSampleEntry
+---------------------------------------------------------------------*/
AP4_AudioSampleEntry::AP4_AudioSampleEntry(AP4_Atom::Type format,
                                           AP4_UI32       sample_rate,
                                           AP4_UI16       sample_size,
                                           AP4_UI16       channel_count) :
    AP4_SampleEntry(format),
    m_QtVersion(0),
    m_QtRevision(0),
    m_QtVendor(0),
    m_ChannelCount(channel_count),
    m_SampleSize(sample_size),
    m_QtCompressionId(0),
    m_QtPacketSize(0),
    m_SampleRate(sample_rate),
    m_QtV1SamplesPerPacket(0),
    m_QtV1BytesPerPacket(0),
    m_QtV1BytesPerFrame(0),
    m_QtV1BytesPerSample(0),
    m_QtV2StructSize(0),
    m_QtV2SampleRate64(0.0),
    m_QtV2ChannelCount(0),
    m_QtV2Reserved(0),
    m_QtV2BitsPerChannel(0),
    m_QtV2FormatSpecificFlags(0),
    m_QtV2BytesPerAudioPacket(0),
    m_QtV2LPCMFramesPerAudioPacket(0)    
{
    m_Size32 += 20;
}

/*----------------------------------------------------------------------
|   AP4_AudioSampleEntry::AP4_AudioSampleEntry
+---------------------------------------------------------------------*/
AP4_AudioSampleEntry::AP4_AudioSampleEntry(AP4_Atom::Type   format,
                                           AP4_Size         size,
                                           AP4_ByteStream&  stream,
                                           AP4_AtomFactory& atom_factory) :
    AP4_SampleEntry(format, size)
{
    Read(stream, atom_factory);
}
    
/*----------------------------------------------------------------------
|   AP4_AudioSampleEntry::GetFieldsSize
+---------------------------------------------------------------------*/
AP4_Size
AP4_AudioSampleEntry::GetFieldsSize()
{
    AP4_Size size = AP4_SampleEntry::GetFieldsSize()+20;
    if (m_QtVersion == 1) {
        size += 16;
    } else if (m_QtVersion == 2) {
        size += 36+m_QtV2Extension.GetDataSize();
    }
    
    return size;
}

/*----------------------------------------------------------------------
|   AP4_AudioSampleEntry::GetSampleRate
+---------------------------------------------------------------------*/
AP4_UI32
AP4_AudioSampleEntry::GetSampleRate()
{
    if (m_QtVersion == 2) {
        return (AP4_UI32)(m_QtV2SampleRate64);
    } else {
        return m_SampleRate>>16;
    }
}

/*----------------------------------------------------------------------
|   AP4_AudioSampleEntry::GetChannelCount
+---------------------------------------------------------------------*/
AP4_UI16
AP4_AudioSampleEntry::GetChannelCount()
{
    if (m_QtVersion == 2) {
        return (AP4_UI16)m_QtV2ChannelCount;
    } else {
        return m_ChannelCount;
    }
}

/*----------------------------------------------------------------------
|   AP4_AudioSampleEntry::ReadFields
+---------------------------------------------------------------------*/
AP4_Result
AP4_AudioSampleEntry::ReadFields(AP4_ByteStream& stream)
{
    // sample entry
    AP4_Result result = AP4_SampleEntry::ReadFields(stream);
    if (result < 0) return result;

    // read the fields of this class
    stream.ReadUI16(m_QtVersion);
    stream.ReadUI16(m_QtRevision);
    stream.ReadUI32(m_QtVendor);
    stream.ReadUI16(m_ChannelCount);
    stream.ReadUI16(m_SampleSize);
    stream.ReadUI16(m_QtCompressionId);
    stream.ReadUI16(m_QtPacketSize);
    stream.ReadUI32(m_SampleRate);

    // if this is a QT V1 entry, read the extension
    if (m_QtVersion == 1) {
        stream.ReadUI32(m_QtV1SamplesPerPacket);
        stream.ReadUI32(m_QtV1BytesPerPacket);
        stream.ReadUI32(m_QtV1BytesPerFrame);
        stream.ReadUI32(m_QtV1BytesPerSample);
    } else if (m_QtVersion == 2) {
        stream.ReadUI32(m_QtV2StructSize);
        stream.ReadDouble(m_QtV2SampleRate64);
        stream.ReadUI32(m_QtV2ChannelCount);
        stream.ReadUI32(m_QtV2Reserved);
        stream.ReadUI32(m_QtV2BitsPerChannel);
        stream.ReadUI32(m_QtV2FormatSpecificFlags);
        stream.ReadUI32(m_QtV2BytesPerAudioPacket);
        stream.ReadUI32(m_QtV2LPCMFramesPerAudioPacket);
        if (m_QtV2StructSize > 72) {
            unsigned int ext_size = m_QtV2StructSize-72;
            m_QtV2Extension.SetDataSize(ext_size);
            stream.Read(m_QtV2Extension.UseData(), ext_size);
        }
        m_QtV1SamplesPerPacket =
        m_QtV1BytesPerPacket   =
        m_QtV1BytesPerFrame    =
        m_QtV1BytesPerSample   = 0;
    } else {
        m_QtV1SamplesPerPacket         = 0;
        m_QtV1BytesPerPacket           = 0;
        m_QtV1BytesPerFrame            = 0;
        m_QtV1BytesPerSample           = 0;
        m_QtV2StructSize               = 0;
        m_QtV2SampleRate64             = 0.0;
        m_QtV2ChannelCount             = 0;
        m_QtV2Reserved                 = 0;
        m_QtV2BitsPerChannel           = 0;
        m_QtV2FormatSpecificFlags      = 0;
        m_QtV2BytesPerAudioPacket      = 0;
        m_QtV2LPCMFramesPerAudioPacket = 0;
    }
    
    return AP4_SUCCESS;
}

/*----------------------------------------------------------------------
|   AP4_AudioSampleEntry::WriteFields
+---------------------------------------------------------------------*/
AP4_Result
AP4_AudioSampleEntry::WriteFields(AP4_ByteStream& stream)
{
    AP4_Result result;
    
    // write the fields of the base class
    result = AP4_SampleEntry::WriteFields(stream);
    if (AP4_FAILED(result)) return result;

    // QT version
    result = stream.WriteUI16(m_QtVersion);
    if (AP4_FAILED(result)) return result;

    // QT revision
    result = stream.WriteUI16(m_QtRevision);
    if (AP4_FAILED(result)) return result;

    // QT vendor
    result = stream.WriteUI32(m_QtVendor);
    if (AP4_FAILED(result)) return result;

    // channel count
    result = stream.WriteUI16(m_ChannelCount);
    if (AP4_FAILED(result)) return result;
    
    // sample size 
    result = stream.WriteUI16(m_SampleSize);
    if (AP4_FAILED(result)) return result;

    // QT compression ID
    result = stream.WriteUI16(m_QtCompressionId);
    if (AP4_FAILED(result)) return result;

    // QT packet size
    result = stream.WriteUI16(m_QtPacketSize);
    if (AP4_FAILED(result)) return result;

    // sample rate
    result = stream.WriteUI32(m_SampleRate);
    if (AP4_FAILED(result)) return result;

    if (m_QtVersion == 1) {
        result = stream.WriteUI32(m_QtV1SamplesPerPacket);
        if (AP4_FAILED(result)) return result;
        result = stream.WriteUI32(m_QtV1BytesPerPacket);
        if (AP4_FAILED(result)) return result;
        result = stream.WriteUI32(m_QtV1BytesPerFrame);
        if (AP4_FAILED(result)) return result;
        result = stream.WriteUI32(m_QtV1BytesPerSample);
        if (AP4_FAILED(result)) return result;
    } else if (m_QtVersion == 2) {
        stream.WriteUI32(m_QtV2StructSize);
        stream.WriteDouble(m_QtV2SampleRate64);
        stream.WriteUI32(m_QtV2ChannelCount);
        stream.WriteUI32(m_QtV2Reserved);
        stream.WriteUI32(m_QtV2BitsPerChannel);
        stream.WriteUI32(m_QtV2FormatSpecificFlags);
        stream.WriteUI32(m_QtV2BytesPerAudioPacket);
        stream.WriteUI32(m_QtV2LPCMFramesPerAudioPacket);
        if (m_QtV2Extension.GetDataSize()) {
            stream.Write(m_QtV2Extension.GetData(),
                         m_QtV2Extension.GetDataSize());
        }
    }
    
    return result;
}

/*----------------------------------------------------------------------
|   AP4_AudioSampleEntry::InspectFields
+---------------------------------------------------------------------*/
AP4_Result
AP4_AudioSampleEntry::InspectFields(AP4_AtomInspector& inspector)
{
    // dump the fields from the base class
    AP4_SampleEntry::InspectFields(inspector);

    // fields
    inspector.AddField("channel_count", GetChannelCount());
    inspector.AddField("sample_size", GetSampleSize());
    inspector.AddField("sample_rate", GetSampleRate());
    if (m_QtVersion) {
        inspector.AddField("qt_version", m_QtVersion);
    }
    
    return AP4_SUCCESS;
}

/*----------------------------------------------------------------------
|   AP4_AudioSampleEntry::ToSampleDescription
+---------------------------------------------------------------------*/
AP4_SampleDescription*
AP4_AudioSampleEntry::ToSampleDescription()
{
    // create a sample description
    return new AP4_GenericAudioSampleDescription(
        m_Type,
        GetSampleRate(),
        GetSampleSize(),
        GetChannelCount(),
        this);
}

/*----------------------------------------------------------------------
|   AP4_MpegAudioSampleEntry::AP4_MpegAudioSampleEntry
+---------------------------------------------------------------------*/
AP4_MpegAudioSampleEntry::AP4_MpegAudioSampleEntry(
    AP4_UI32          type,
    AP4_UI32          sample_rate, 
    AP4_UI16          sample_size,
    AP4_UI16          channel_count,
    AP4_EsDescriptor* descriptor) :
    AP4_AudioSampleEntry(type, sample_rate, sample_size, channel_count)
{
    if (descriptor) AddChild(new AP4_EsdsAtom(descriptor));
}

/*----------------------------------------------------------------------
|   AP4_MpegAudioSampleEntry::AP4_MpegAudioSampleEntry
+---------------------------------------------------------------------*/
AP4_MpegAudioSampleEntry::AP4_MpegAudioSampleEntry(
    AP4_UI32         type,
    AP4_Size         size,
    AP4_ByteStream&  stream,
    AP4_AtomFactory& atom_factory) :
    AP4_AudioSampleEntry(type, size, stream, atom_factory)
{
}

/*----------------------------------------------------------------------
|   AP4_MpegAudioSampleEntry::ToSampleDescription
+---------------------------------------------------------------------*/
AP4_SampleDescription*
AP4_MpegAudioSampleEntry::ToSampleDescription()
{
    // find the esds atom
    AP4_EsdsAtom* esds = AP4_DYNAMIC_CAST(AP4_EsdsAtom, GetChild(AP4_ATOM_TYPE_ESDS));
    if (esds == NULL) {
        // check if this is a quicktime style sample description
        if (m_QtVersion > 0) {
            esds = AP4_DYNAMIC_CAST(AP4_EsdsAtom, FindChild("wave/esds"));
        }
    }
    
    // create a sample description
    return new AP4_MpegAudioSampleDescription(GetSampleRate(),
                                              GetSampleSize(),
                                              GetChannelCount(),
                                              esds);
}

/*----------------------------------------------------------------------
|   AP4_Mp4aSampleEntry::AP4_Mp4aSampleEntry
+---------------------------------------------------------------------*/
AP4_Mp4aSampleEntry::AP4_Mp4aSampleEntry(AP4_UI32          sample_rate, 
                                         AP4_UI16          sample_size,
                                         AP4_UI16          channel_count,
                                         AP4_EsDescriptor* descriptor) :
    AP4_MpegAudioSampleEntry(AP4_ATOM_TYPE_MP4A, 
                             sample_rate, 
                             sample_size, 
                             channel_count,
                             descriptor)
{
}

/*----------------------------------------------------------------------
|   AP4_Mp4aSampleEntry::AP4_Mp4aSampleEntry
+---------------------------------------------------------------------*/
AP4_Mp4aSampleEntry::AP4_Mp4aSampleEntry(AP4_Size         size,
                                         AP4_ByteStream&  stream,
                                         AP4_AtomFactory& atom_factory) :
    AP4_MpegAudioSampleEntry(AP4_ATOM_TYPE_MP4A, size, stream, atom_factory)
{
}

/*----------------------------------------------------------------------
|   AP4_VisualSampleEntry::AP4_VisualSampleEntry
+---------------------------------------------------------------------*/
AP4_VisualSampleEntry::AP4_VisualSampleEntry(
    AP4_Atom::Type    format, 
    AP4_UI16          width,
    AP4_UI16          height,
    AP4_UI16          depth,
    const char*           compressor_name,
    const AP4_AtomParent* details) :
    AP4_SampleEntry(format, details),
    m_Predefined1(0),
    m_Reserved2(0),
    m_Width(width),
    m_Height(height),
    m_HorizResolution(0x00480000),
    m_VertResolution(0x00480000),
    m_Reserved3(0),
    m_FrameCount(1),
    m_CompressorName(compressor_name),
    m_Depth(depth),
    m_Predefined3(0xFFFF)
{
    memset(m_Predefined2, 0, sizeof(m_Predefined2));
    m_Size32 += 70;
}

/*----------------------------------------------------------------------
|   AP4_VisualSampleEntry::AP4_VisualSampleEntry
+---------------------------------------------------------------------*/
AP4_VisualSampleEntry::AP4_VisualSampleEntry(AP4_Atom::Type   format,
                                             AP4_Size         size, 
                                             AP4_ByteStream&  stream,
                                             AP4_AtomFactory& atom_factory) :
    AP4_SampleEntry(format, size)
{
    Read(stream, atom_factory);
}

/*----------------------------------------------------------------------
|   AP4_VisualSampleEntry::GetFieldsSize
+---------------------------------------------------------------------*/
AP4_Size
AP4_VisualSampleEntry::GetFieldsSize()
{
    return AP4_SampleEntry::GetFieldsSize()+70;
}

/*----------------------------------------------------------------------
|   AP4_VisualSampleEntry::ReadFields
+---------------------------------------------------------------------*/
AP4_Result
AP4_VisualSampleEntry::ReadFields(AP4_ByteStream& stream)
{
    // sample entry
    AP4_Result result = AP4_SampleEntry::ReadFields(stream);
    if (result < 0) return result;

    // read fields from this class
    stream.ReadUI16(m_Predefined1);
    stream.ReadUI16(m_Reserved2);
    stream.Read(m_Predefined2, sizeof(m_Predefined2));
    stream.ReadUI16(m_Width);
    stream.ReadUI16(m_Height);
    stream.ReadUI32(m_HorizResolution);
    stream.ReadUI32(m_VertResolution);
    stream.ReadUI32(m_Reserved3);
    stream.ReadUI16(m_FrameCount);

    AP4_UI08 compressor_name[33];
    compressor_name[32] = 0;
    stream.Read(compressor_name, 32);
    AP4_UI08 name_length = compressor_name[0];
    if (name_length < 32) {
        compressor_name[name_length+1] = 0; // force null termination
        m_CompressorName = (const char*)(&compressor_name[1]);
    }

    stream.ReadUI16(m_Depth);
    stream.ReadUI16(m_Predefined3);

    return AP4_SUCCESS;
}

/*----------------------------------------------------------------------
|   AP4_VisualSampleEntry::WriteFields
+---------------------------------------------------------------------*/
AP4_Result
AP4_VisualSampleEntry::WriteFields(AP4_ByteStream& stream)
{
    AP4_Result result;
        
    // write the fields of the base class
    result = AP4_SampleEntry::WriteFields(stream);
    if (AP4_FAILED(result)) return result;

    // predefined1
    result = stream.WriteUI16(m_Predefined1);
    if (AP4_FAILED(result)) return result;
    
    // reserved2
    result = stream.WriteUI16(m_Reserved2);
    if (AP4_FAILED(result)) return result;
    
    // predefined2
    result = stream.Write(m_Predefined2, sizeof(m_Predefined2));
    if (AP4_FAILED(result)) return result;
    
    // width
    result = stream.WriteUI16(m_Width);
    if (AP4_FAILED(result)) return result;
    
    // height
    result = stream.WriteUI16(m_Height);
    if (AP4_FAILED(result)) return result;
    
    // horizontal resolution
    result = stream.WriteUI32(m_HorizResolution);
    if (AP4_FAILED(result)) return result;
    
    // vertical resolution
    result = stream.WriteUI32(m_VertResolution);
    if (AP4_FAILED(result)) return result;
    
    // reserved3
    result = stream.WriteUI32(m_Reserved3);
    if (AP4_FAILED(result)) return result;
    
    // frame count
    result = stream.WriteUI16(m_FrameCount);
    if (AP4_FAILED(result)) return result;
    
    // compressor name
    unsigned char compressor_name[32];
    unsigned int name_length = m_CompressorName.GetLength();
    if (name_length > 31) name_length = 31;
    compressor_name[0] = (unsigned char)name_length;
    for (unsigned int i=0; i<name_length; i++) {
        compressor_name[i+1] = m_CompressorName[i];
    }
    for (unsigned int i=name_length+1; i<32; i++) {
        compressor_name[i] = 0;
    }
    result = stream.Write(compressor_name, 32);
    if (AP4_FAILED(result)) return result;
    
    // depth
    result = stream.WriteUI16(m_Depth);
    if (AP4_FAILED(result)) return result;
    
    // predefined3
    result = stream.WriteUI16(m_Predefined3);
    if (AP4_FAILED(result)) return result;
    
    return result;
}

/*----------------------------------------------------------------------
|   AP4_VisualSampleEntry::InspectFields
+---------------------------------------------------------------------*/
AP4_Result
AP4_VisualSampleEntry::InspectFields(AP4_AtomInspector& inspector)
{
    // dump the fields of the base class
    AP4_SampleEntry::InspectFields(inspector);

    // fields
    inspector.AddField("width", m_Width);
    inspector.AddField("height", m_Height);
    inspector.AddField("compressor", m_CompressorName.GetChars());

    return AP4_SUCCESS;
}

/*----------------------------------------------------------------------
|   AP4_VisualSampleEntry::ToSampleDescription
+---------------------------------------------------------------------*/
AP4_SampleDescription*
AP4_VisualSampleEntry::ToSampleDescription()
{
    // create a sample description
    return new AP4_GenericVideoSampleDescription(
        m_Type,
        m_Width,
        m_Height,
        m_Depth,
        m_CompressorName.GetChars(),
        this);
}

/*----------------------------------------------------------------------
|   AP4_MpegVideoSampleEntry::AP4_MpegVideoSampleEntry
+---------------------------------------------------------------------*/
AP4_MpegVideoSampleEntry::AP4_MpegVideoSampleEntry(
    AP4_UI32          type,
    AP4_UI16          width,
    AP4_UI16          height,
    AP4_UI16          depth,
    const char*       compressor_name,
    AP4_EsDescriptor* descriptor) :
    AP4_VisualSampleEntry(type, 
                          width, 
                          height, 
                          depth, 
                          compressor_name)
{
    if (descriptor) AddChild(new AP4_EsdsAtom(descriptor));
}

/*----------------------------------------------------------------------
|   AP4_MpegVideoSampleEntry::AP4_MpegVideoSampleEntry
+---------------------------------------------------------------------*/
AP4_MpegVideoSampleEntry::AP4_MpegVideoSampleEntry(
    AP4_UI32         type,
    AP4_Size         size,
    AP4_ByteStream&  stream,
    AP4_AtomFactory& atom_factory) :
    AP4_VisualSampleEntry(type, size, stream, atom_factory)
{
}

/*----------------------------------------------------------------------
|   AP4_MpegVideoSampleEntry::ToSampleDescription
+---------------------------------------------------------------------*/
AP4_SampleDescription*
AP4_MpegVideoSampleEntry::ToSampleDescription()
{
    // create a sample description
    return new AP4_MpegVideoSampleDescription(
        m_Width,
        m_Height,
        m_Depth,
        m_CompressorName.GetChars(),
        AP4_DYNAMIC_CAST(AP4_EsdsAtom, GetChild(AP4_ATOM_TYPE_ESDS)));
}

/*----------------------------------------------------------------------
|   AP4_Mp4vSampleEntry::AP4_Mp4vSampleEntry
+---------------------------------------------------------------------*/
AP4_Mp4vSampleEntry::AP4_Mp4vSampleEntry(AP4_UI16          width,
                                         AP4_UI16          height,
                                         AP4_UI16          depth,
                                         const char*       compressor_name,
                                         AP4_EsDescriptor* descriptor) :
    AP4_MpegVideoSampleEntry(AP4_ATOM_TYPE_MP4V, 
                             width, 
                             height, 
                             depth, 
                             compressor_name,
                             descriptor)
{
}

/*----------------------------------------------------------------------
|   AP4_Mp4vSampleEntry::AP4_Mp4aSampleEntry
+---------------------------------------------------------------------*/
AP4_Mp4vSampleEntry::AP4_Mp4vSampleEntry(AP4_Size         size,
                                         AP4_ByteStream&  stream,
                                         AP4_AtomFactory& atom_factory) :
    AP4_MpegVideoSampleEntry(AP4_ATOM_TYPE_MP4V, size, stream, atom_factory)
{
}

/*----------------------------------------------------------------------
|   AP4_AvcSampleEntry::AP4_AvcSSampleEntry
+---------------------------------------------------------------------*/
AP4_AvcSampleEntry::AP4_AvcSampleEntry(AP4_UI32            format,
                                       AP4_UI16            width,
                                       AP4_UI16            height,
                                       AP4_UI16            depth,
                                       const char*         compressor_name,
                                       const AP4_AtomParent* details) :
    AP4_VisualSampleEntry(format,
                          width, 
                          height, 
                          depth, 
                          compressor_name,
                          details)
{
}

/*----------------------------------------------------------------------
|   AP4_AvcSampleEntry::AP4_AvcSampleEntry
+---------------------------------------------------------------------*/
AP4_AvcSampleEntry::AP4_AvcSampleEntry(AP4_UI32         format,
                                       AP4_Size         size,
                                       AP4_ByteStream&  stream,
                                       AP4_AtomFactory& atom_factory) :
    AP4_VisualSampleEntry(format, size, stream, atom_factory)
{
}

/*----------------------------------------------------------------------
|   AP4_HevcSampleEntry::AP4_HevcSSampleEntry
+---------------------------------------------------------------------*/
AP4_HevcSampleEntry::AP4_HevcSampleEntry(AP4_UI32            format,
                                         AP4_UI16            width,
                                         AP4_UI16            height,
                                         AP4_UI16            depth,
                                         const char*         compressor_name,
                                         const AP4_AtomParent* details) :
    AP4_VisualSampleEntry(format,
                          width, 
                          height, 
                          depth, 
                          compressor_name,
                          details)
{
}

/*----------------------------------------------------------------------
|   AP4_AvcSampleEntry::ToSampleDescription
+---------------------------------------------------------------------*/
AP4_SampleDescription*
AP4_AvcSampleEntry::ToSampleDescription()
{
    return new AP4_AvcSampleDescription(
        m_Type,
        m_Width,
        m_Height,
        m_Depth,
        m_CompressorName.GetChars(),
        this);
}

/*----------------------------------------------------------------------
|   AP4_HevcSampleEntry::AP4_HevcSampleEntry
+---------------------------------------------------------------------*/
AP4_HevcSampleEntry::AP4_HevcSampleEntry(AP4_UI32         format,
                                         AP4_Size         size,
                                         AP4_ByteStream&  stream,
                                         AP4_AtomFactory& atom_factory) :
    AP4_VisualSampleEntry(format, size, stream, atom_factory)
{
}

/*----------------------------------------------------------------------
|   AP4_HevcSampleEntry::ToSampleDescription
+---------------------------------------------------------------------*/
AP4_SampleDescription*
AP4_HevcSampleEntry::ToSampleDescription()
{
    return new AP4_HevcSampleDescription(
        m_Type,
        m_Width,
        m_Height,
        m_Depth,
        m_CompressorName.GetChars(),
        this);
}

/*----------------------------------------------------------------------
|   AP4_RtpHintSampleEntry::AP4_RtpHintSampleEntry
+---------------------------------------------------------------------*/
AP4_RtpHintSampleEntry::AP4_RtpHintSampleEntry(AP4_UI16 hint_track_version,
                                               AP4_UI16 highest_compatible_version,
                                               AP4_UI32 max_packet_size,
                                               AP4_UI32 timescale):
    AP4_SampleEntry(AP4_ATOM_TYPE_RTP_),
    m_HintTrackVersion(hint_track_version),
    m_HighestCompatibleVersion(highest_compatible_version),
    m_MaxPacketSize(max_packet_size)
{
    // build an atom for timescale
    AddChild(new AP4_TimsAtom(timescale));
}

/*----------------------------------------------------------------------
|   AP4_RtpHintSampleEntry::AP4_RtpHintSampleEntry
+---------------------------------------------------------------------*/
AP4_RtpHintSampleEntry::AP4_RtpHintSampleEntry(AP4_Size         size,
                                               AP4_ByteStream&  stream,
                                               AP4_AtomFactory& atom_factory): 
    AP4_SampleEntry(AP4_ATOM_TYPE_RTP_, size)
{
    Read(stream, atom_factory);
}

/*----------------------------------------------------------------------
|   AP4_RtpHintSampleEntry::GetFieldsSize
+---------------------------------------------------------------------*/
AP4_Size
AP4_RtpHintSampleEntry::GetFieldsSize()
{
    return AP4_SampleEntry::GetFieldsSize()+8;
}

/*----------------------------------------------------------------------
|   AP4_RtpHintSampleEntry::ReadFields
+---------------------------------------------------------------------*/
AP4_Result
AP4_RtpHintSampleEntry::ReadFields(AP4_ByteStream& stream)
{
    // sample entry
    AP4_Result result = AP4_SampleEntry::ReadFields(stream);
    if (result < 0) return result;

    // data
    result = stream.ReadUI16(m_HintTrackVersion);
    if (AP4_FAILED(result)) return result;
    result = stream.ReadUI16(m_HighestCompatibleVersion);
    if (AP4_FAILED(result)) return result;
    result = stream.ReadUI32(m_MaxPacketSize);
    if (AP4_FAILED(result)) return result;

    return AP4_SUCCESS;
}

/*----------------------------------------------------------------------
|   AP4_RtpHintSampleEntry::WriteFields
+---------------------------------------------------------------------*/
AP4_Result
AP4_RtpHintSampleEntry::WriteFields(AP4_ByteStream& stream)
{
    // sample entry
    AP4_Result result = AP4_SampleEntry::WriteFields(stream);
    if (AP4_FAILED(result)) return result;
    
    // data
    result = stream.WriteUI16(m_HintTrackVersion);
    if (AP4_FAILED(result)) return result;
    result = stream.WriteUI16(m_HighestCompatibleVersion);
    if (AP4_FAILED(result)) return result;
    result = stream.WriteUI32(m_MaxPacketSize);
    if (AP4_FAILED(result)) return result;

    return result;
}

/*----------------------------------------------------------------------
|   AP4_RtpHintSampleEntry::InspectFields
+---------------------------------------------------------------------*/
AP4_Result
AP4_RtpHintSampleEntry::InspectFields(AP4_AtomInspector& inspector)
{
    // sample entry
    AP4_SampleEntry::InspectFields(inspector);
    
    // fields
    inspector.AddField("hint_track_version", m_HintTrackVersion);
    inspector.AddField("highest_compatible_version", m_HighestCompatibleVersion);
    inspector.AddField("max_packet_size", m_MaxPacketSize);
    
    return AP4_SUCCESS;
}

/*----------------------------------------------------------------------
|   AP4_SubtitleSampleEntry::AP4_SubtitleSampleEntry
+---------------------------------------------------------------------*/
AP4_SubtitleSampleEntry::AP4_SubtitleSampleEntry(
    AP4_Atom::Type    format,
    const char*       namespce,
    const char*       schema_location,
    const char*       image_mime_type) :
    AP4_SampleEntry(format),
    m_Namespace(namespce),
    m_SchemaLocation(schema_location),
    m_ImageMimeType(image_mime_type)
{
    SetSize(m_Size32+m_Namespace.GetLength()+1+m_SchemaLocation.GetLength()+1+m_ImageMimeType.GetLength()+1);
}

/*----------------------------------------------------------------------
|   AP4_SubtitleSampleEntry::AP4_SubtitleSampleEntry
+---------------------------------------------------------------------*/
AP4_SubtitleSampleEntry::AP4_SubtitleSampleEntry(AP4_Atom::Type   format,
                                                 AP4_Size         size,
                                                 AP4_ByteStream&  stream,
                                                 AP4_AtomFactory& atom_factory) :
    AP4_SampleEntry(format, size)
{
    Read(stream, atom_factory);
}

/*----------------------------------------------------------------------
|   AP4_SubtitleSampleEntry::GetFieldsSize
+---------------------------------------------------------------------*/
AP4_Size
AP4_SubtitleSampleEntry::GetFieldsSize()
{
    return AP4_SampleEntry::GetFieldsSize() +
        3                                   +
        m_Namespace.GetLength()             +
        m_SchemaLocation.GetLength()        +
        m_ImageMimeType.GetLength();
}

/*----------------------------------------------------------------------
|   AP4_SubtitleSampleEntry::ReadFields
+---------------------------------------------------------------------*/
AP4_Result
AP4_SubtitleSampleEntry::ReadFields(AP4_ByteStream& stream)
{
    // sample entry
    AP4_Result result = AP4_SampleEntry::ReadFields(stream);
    if (result < 0) return result;

    // read fields from this class
    result = stream.ReadNullTerminatedString(m_Namespace);
    if (AP4_FAILED(result)) return result;
    result = stream.ReadNullTerminatedString(m_SchemaLocation);
    if (AP4_FAILED(result)) return result;
    result = stream.ReadNullTerminatedString(m_ImageMimeType);
    if (AP4_FAILED(result)) return result;
    
    return AP4_SUCCESS;
}

/*----------------------------------------------------------------------
|   AP4_SubtitleSampleEntry::WriteFields
+---------------------------------------------------------------------*/
AP4_Result
AP4_SubtitleSampleEntry::WriteFields(AP4_ByteStream& stream)
{
    AP4_Result result;
        
    // write the fields of the base class
    result = AP4_SampleEntry::WriteFields(stream);
    if (AP4_FAILED(result)) return result;
    
    // write fields from this class
    result = stream.WriteString(m_Namespace.GetChars());
    if (AP4_FAILED(result)) return result;
    result = stream.WriteUI08(0);
    if (AP4_FAILED(result)) return result;
    result = stream.WriteString(m_SchemaLocation.GetChars());
    if (AP4_FAILED(result)) return result;
    result = stream.WriteUI08(0);
    if (AP4_FAILED(result)) return result;
    result = stream.WriteString(m_ImageMimeType.GetChars());
    if (AP4_FAILED(result)) return result;
    result = stream.WriteUI08(0);
    if (AP4_FAILED(result)) return result;
    
    return AP4_SUCCESS;
}

/*----------------------------------------------------------------------
|   AP4_SubtitleSampleEntry::InspectFields
+---------------------------------------------------------------------*/
AP4_Result
AP4_SubtitleSampleEntry::InspectFields(AP4_AtomInspector& inspector)
{
    // dump the fields of the base class
    AP4_SampleEntry::InspectFields(inspector);

    // fields
    inspector.AddField("namespace",       m_Namespace.GetChars());
    inspector.AddField("schema_location", m_SchemaLocation.GetChars());
    inspector.AddField("image_mime_type", m_ImageMimeType.GetChars());

    return AP4_SUCCESS;
}

/*----------------------------------------------------------------------
|   AP4_SubtitleSampleEntry::ToSampleDescription
+---------------------------------------------------------------------*/
AP4_SampleDescription*
AP4_SubtitleSampleEntry::ToSampleDescription()
{
    // create a sample description
    return new AP4_SubtitleSampleDescription(m_Type,
                                             m_Namespace.GetChars(),
                                             m_SchemaLocation.GetChars(),
                                             m_ImageMimeType.GetChars());
}

