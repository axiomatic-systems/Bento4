/*****************************************************************
|
|    AP4 - MP4 File Info
|
|    Copyright 2002-2015 Axiomatic Systems, LLC
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
#include <stdio.h>
#include <stdlib.h>

#include "Ap4.h"
#include "Ap4BitStream.h"
#include "Ap4Mp4AudioInfo.h"
#include "Ap4HevcParser.h"

/*----------------------------------------------------------------------
|   constants
+---------------------------------------------------------------------*/
#define BANNER "MP4 File Info - Version 1.3.4\n"\
               "(Bento4 Version " AP4_VERSION_STRING ")\n"\
               "(c) 2002-2017 Axiomatic Systems, LLC"
 
/*----------------------------------------------------------------------
|   globals
+---------------------------------------------------------------------*/
typedef struct {
    AP4_UI64 sample_count;
    AP4_UI64 duration;
    double   bitrate;
} MediaInfo;

typedef enum {
    TEXT_FORMAT,
    JSON_FORMAT
} OutputFormat;

struct Options {
    OutputFormat format;
} Options;

/*----------------------------------------------------------------------
|   PrintUsageAndExit
+---------------------------------------------------------------------*/
static void
PrintUsageAndExit()
{
    fprintf(stderr, 
            BANNER 
            "\n\nusage: mp4info [options] <input>\n"
            "Options:\n"
            "  --verbose:          show extended information when available\n"
            "  --format <format>:  display the information in this format.\n"
            "                      <format> is: text (default) or json\n"
            "  --show-layout:      show sample layout\n"
            "  --show-samples:     show sample details\n"
            "  --show-sample-data: show sample data\n"
            "  --fast:             skip some details that are slow to compute\n");
    exit(1);
}

/*----------------------------------------------------------------------
|   ShowPayload
+---------------------------------------------------------------------*/
static void
ShowPayload(AP4_Atom& atom, bool ascii = false)
{
    AP4_UI64 payload_size = atom.GetSize()-8;
    if (payload_size <= 1024) {
        AP4_MemoryByteStream* payload = new AP4_MemoryByteStream();
        atom.Write(*payload);
        if (ascii) {
            // ascii
            payload->WriteUI08(0); // terminate with a NULL character
            printf("%s", (const char*)payload->GetData()+atom.GetHeaderSize());
        } else {
            // hex
            for (unsigned int i=0; i<payload_size; i++) {
                printf("%02x", (unsigned char)payload->GetData()[atom.GetHeaderSize()+i]);
            }
        }
        payload->Release();
    }
}

/*----------------------------------------------------------------------
|   ShowData
+---------------------------------------------------------------------*/
static void
ShowData(const AP4_DataBuffer& data)
{
    for (unsigned int i=0; i<data.GetDataSize(); i++) {
        printf("%02x", (unsigned char)data.GetData()[i]);
    }
}

/*----------------------------------------------------------------------
|   ShowProtectionSchemeInfo_Text
+---------------------------------------------------------------------*/
static void
ShowProtectionSchemeInfo_Text(AP4_UI32 scheme_type, AP4_ContainerAtom& schi, bool verbose)
{
    if (scheme_type == AP4_PROTECTION_SCHEME_TYPE_IAEC) {
        printf("      iAEC Scheme Info:\n");
        AP4_IkmsAtom* ikms = AP4_DYNAMIC_CAST(AP4_IkmsAtom, schi.FindChild("iKMS"));
        if (ikms) {
            printf("        KMS URI:              %s\n", ikms->GetKmsUri().GetChars());
        }
        AP4_IsfmAtom* isfm = AP4_DYNAMIC_CAST(AP4_IsfmAtom, schi.FindChild("iSFM"));
        if (isfm) {
            printf("        Selective Encryption: %s\n", isfm->GetSelectiveEncryption()?"yes":"no");
            printf("        Key Indicator Length: %d\n", isfm->GetKeyIndicatorLength());
            printf("        IV Length:            %d\n", isfm->GetIvLength());
        }
        AP4_IsltAtom* islt = AP4_DYNAMIC_CAST(AP4_IsltAtom, schi.FindChild("iSLT"));
        if (islt) {
            printf("        Salt:                 ");
            for (unsigned int i=0; i<8; i++) {
                printf("%02x",islt->GetSalt()[i]);
            }
            printf("\n");
        }
    } else if (scheme_type == AP4_PROTECTION_SCHEME_TYPE_OMA) {
        printf("      odkm Scheme Info:\n");
        AP4_OdafAtom* odaf = AP4_DYNAMIC_CAST(AP4_OdafAtom, schi.FindChild("odkm/odaf"));
        if (odaf) {
            printf("        Selective Encryption: %s\n", odaf->GetSelectiveEncryption()?"yes":"no");
            printf("        Key Indicator Length: %d\n", odaf->GetKeyIndicatorLength());
            printf("        IV Length:            %d\n", odaf->GetIvLength());
        }
        AP4_OhdrAtom* ohdr = AP4_DYNAMIC_CAST(AP4_OhdrAtom, schi.FindChild("odkm/ohdr"));
        if (ohdr) {
            const char* encryption_method = "";
            switch (ohdr->GetEncryptionMethod()) {
                case AP4_OMA_DCF_ENCRYPTION_METHOD_NULL:    encryption_method = "NULL";    break;
                case AP4_OMA_DCF_ENCRYPTION_METHOD_AES_CTR: encryption_method = "AES-CTR"; break;
                case AP4_OMA_DCF_ENCRYPTION_METHOD_AES_CBC: encryption_method = "AES-CBC"; break;
                default:                                    encryption_method = "UNKNOWN"; break;
            }
            printf("        Encryption Method: %s\n", encryption_method);
            printf("        Content ID:        %s\n", ohdr->GetContentId().GetChars());
            printf("        Rights Issuer URL: %s\n", ohdr->GetRightsIssuerUrl().GetChars());
            
            const AP4_DataBuffer& headers = ohdr->GetTextualHeaders();
            AP4_Size              data_len    = headers.GetDataSize();
            if (data_len) {
                AP4_Byte*      textual_headers_string;
                AP4_Byte*      curr;
                AP4_DataBuffer output_buffer;
                output_buffer.SetDataSize(data_len+1);
                AP4_CopyMemory(output_buffer.UseData(), headers.GetData(), data_len);
                curr = textual_headers_string = output_buffer.UseData();
                textual_headers_string[data_len] = '\0';
                while(curr < textual_headers_string+data_len) {
                    if ('\0' == *curr) {
                        *curr = '\n';
                    }
                    curr++;
                }
                printf("        Textual Headers: \n%s\n", (const char*)textual_headers_string);
            }
        }
    } else if (scheme_type == AP4_PROTECTION_SCHEME_TYPE_ITUNES) {
        printf("      itun Scheme Info:\n");
        AP4_Atom* name = schi.FindChild("name");
        if (name) {
            printf("        Name:    ");
            ShowPayload(*name, true);
            printf("\n");
        }
        AP4_Atom* user = schi.FindChild("user");
        if (user) {
            printf("        User ID: ");
            ShowPayload(*user);
            printf("\n");
        }
        AP4_Atom* key = schi.FindChild("key ");
        if (key) {
            printf("        Key ID:  ");
            ShowPayload(*key);
            printf("\n");
        }
        AP4_Atom* iviv = schi.FindChild("iviv");
        if (iviv) {
            printf("        IV:      ");
            ShowPayload(*iviv);
            printf("\n");
        }
    } else if (scheme_type == AP4_PROTECTION_SCHEME_TYPE_MARLIN_ACBC ||
               scheme_type == AP4_PROTECTION_SCHEME_TYPE_MARLIN_ACGK) {
        printf("      Marlin IPMP ACBC/ACGK Scheme Info:\n");
        AP4_NullTerminatedStringAtom* octopus_id = AP4_DYNAMIC_CAST(AP4_NullTerminatedStringAtom, schi.FindChild("8id "));
        if (octopus_id) {
            printf("        Content ID: %s\n", octopus_id->GetValue().GetChars());
        }
    }
    
    if (verbose) {
        printf("    Protection System Details:\n");
        AP4_ByteStream* output = NULL;
        AP4_FileByteStream::Create("-stdout", AP4_FileByteStream::STREAM_MODE_WRITE, output);
        AP4_PrintInspector inspector(*output, 4);
        schi.Inspect(inspector);
        output->Release();
    }
}

/*----------------------------------------------------------------------
|   ShowProtectionSchemeInfo_Json
+---------------------------------------------------------------------*/
static void
ShowProtectionSchemeInfo_Json(AP4_UI32 /* scheme_type */, AP4_ContainerAtom& /* schi */, bool /* verbose */)
{
    // not implemented yet
}

/*----------------------------------------------------------------------
|   ShowProtectedSampleDescription_Text
+---------------------------------------------------------------------*/
static void
ShowProtectedSampleDescription_Text(AP4_ProtectedSampleDescription& desc, bool verbose)
{
    printf("    [ENCRYPTED]\n");
    char coding[5];
    AP4_FormatFourChars(coding, desc.GetFormat());
    printf("      Coding:         %s\n", coding);
    AP4_UI32 st = desc.GetSchemeType();
    printf("      Scheme Type:    %c%c%c%c\n", 
        (char)((st>>24) & 0xFF),
        (char)((st>>16) & 0xFF),
        (char)((st>> 8) & 0xFF),
        (char)((st    ) & 0xFF));
    printf("      Scheme Version: %d\n", desc.GetSchemeVersion());
    printf("      Scheme URI:     %s\n", desc.GetSchemeUri().GetChars());
    AP4_ProtectionSchemeInfo* scheme_info = desc.GetSchemeInfo();
    if (scheme_info == NULL) return;
    AP4_ContainerAtom* schi = scheme_info->GetSchiAtom();
    if (schi == NULL) return;
    ShowProtectionSchemeInfo_Text(desc.GetSchemeType(), *schi, verbose);
}

/*----------------------------------------------------------------------
|   ShowProtectedSampleDescription_Json
+---------------------------------------------------------------------*/
static void
ShowProtectedSampleDescription_Json(AP4_ProtectedSampleDescription& desc, bool verbose)
{
    printf("\"protection\":{\n");
    char coding[5];
    AP4_FormatFourChars(coding, desc.GetFormat());
    printf("\"coding\":\"%s\",\n", coding);
    AP4_UI32 st = desc.GetSchemeType();
    printf("\"scheme_type\":\"%c%c%c%c\",\n", 
        (char)((st>>24) & 0xFF),
        (char)((st>>16) & 0xFF),
        (char)((st>> 8) & 0xFF),
        (char)((st    ) & 0xFF));
    printf("\"scheme_version\":%d,\n", desc.GetSchemeVersion());
    printf("\"scheme_uri\":\"%s\"", desc.GetSchemeUri().GetChars());
    AP4_ProtectionSchemeInfo* scheme_info = desc.GetSchemeInfo();
    if (scheme_info) {
        AP4_ContainerAtom* schi = scheme_info->GetSchiAtom();
        if (schi) {
            ShowProtectionSchemeInfo_Json(desc.GetSchemeType(), *schi, verbose);
        }
    }
    printf("}");
}

/*----------------------------------------------------------------------
|   ShowMpegAudioSampleDescription
+---------------------------------------------------------------------*/
static void
ShowMpegAudioSampleDescription(AP4_MpegAudioSampleDescription& mpeg_audio_desc)
{
    AP4_MpegAudioSampleDescription::Mpeg4AudioObjectType object_type = 
        mpeg_audio_desc.GetMpeg4AudioObjectType();
    const char* object_type_string = AP4_MpegAudioSampleDescription::GetMpeg4AudioObjectTypeString(object_type);
    AP4_String codec_string;
    mpeg_audio_desc.GetCodecString(codec_string);
    
    switch (Options.format) {
        case TEXT_FORMAT:
            printf("    Codecs String: %s\n", codec_string.GetChars());
            printf("    MPEG-4 Audio Object Type: %d (%s)\n", object_type, object_type_string);
            break;

        case JSON_FORMAT:
            printf("\"codecs_string\": \"%s\",\n", codec_string.GetChars());
            printf("\"mpeg_4_audio_object_type\":%d,\n",          object_type);
            printf("\"mpeg_4_audio_object_type_name\":\"%s\"", object_type_string);
            break;
    }
    
    // Decoder Specific Info
    const AP4_DataBuffer& dsi = mpeg_audio_desc.GetDecoderInfo();
    if (dsi.GetDataSize()) {
        AP4_Mp4AudioDecoderConfig dec_config;
        AP4_Result result = dec_config.Parse(dsi.GetData(), dsi.GetDataSize());
        if (AP4_SUCCEEDED(result)) {
            switch (Options.format) {
                case TEXT_FORMAT:
                    printf("    MPEG-4 Audio Decoder Config:\n");
                    printf("      Sampling Frequency: %d\n", dec_config.m_SamplingFrequency);
                    printf("      Channels: %d\n", dec_config.m_ChannelCount);
                    if (dec_config.m_Extension.m_ObjectType) {
                        object_type_string = AP4_MpegAudioSampleDescription::GetMpeg4AudioObjectTypeString(
                            dec_config.m_Extension.m_ObjectType);

                        printf("      Extension:\n");
                        printf("        Object Type: %s\n", object_type_string);
                        printf("        SBR Present: %s\n", dec_config.m_Extension.m_SbrPresent?"yes":"no");
                        printf("        PS Present:  %s\n", dec_config.m_Extension.m_PsPresent?"yes":"no");
                        printf("        Sampling Frequency: %d\n", dec_config.m_Extension.m_SamplingFrequency);
                    }
                    break;
    
                case JSON_FORMAT:
                    printf(",\n");
                    printf("\"mpeg_4_audio_decoder_config\":{\n");
                    printf("  \"sampling_frequency\":%d,\n", dec_config.m_SamplingFrequency);
                    printf("  \"channels\":%d\n", dec_config.m_ChannelCount);
                    if (dec_config.m_Extension.m_ObjectType) {
                        object_type_string = AP4_MpegAudioSampleDescription::GetMpeg4AudioObjectTypeString(
                            dec_config.m_Extension.m_ObjectType);

                        printf(",\n");
                        printf("  \"extension\":{\n");
                        printf("    \"object_type\":%d,\n", dec_config.m_Extension.m_ObjectType);
                        printf("    \"object_type_name\":\"%s\",\n", object_type_string);
                        printf("    \"sbr_present\":%s,\n", dec_config.m_Extension.m_SbrPresent?"true":"false");
                        printf("    \"ps_present\":%s,\n",   dec_config.m_Extension.m_PsPresent?"true":"false");
                        printf("    \"sampling_frequency\":%d\n", dec_config.m_Extension.m_SamplingFrequency);
                        printf("  }\n");
                    }
                    printf("}");
                    break;
            }
        }
    }
}

/*----------------------------------------------------------------------
|   ShowSampleDescription_Text
+---------------------------------------------------------------------*/
static void
ShowSampleDescription_Text(AP4_SampleDescription& description, bool verbose)
{
    AP4_SampleDescription* desc = &description;
    if (desc->GetType() == AP4_SampleDescription::TYPE_PROTECTED) {
        AP4_ProtectedSampleDescription* prot_desc = AP4_DYNAMIC_CAST(AP4_ProtectedSampleDescription, desc);
        if (prot_desc) {
            ShowProtectedSampleDescription_Text(*prot_desc, verbose);
            desc = prot_desc->GetOriginalSampleDescription();
        }
    }
    
    if (verbose) {
        printf("    Bytes: ");
        AP4_Atom* details = desc->ToAtom();
        ShowPayload(*details, false);
        printf("\n");
        delete details;
    }
    
    char coding[5];
    AP4_FormatFourChars(coding, desc->GetFormat());
    printf(    "    Coding:      %s", coding);
    const char* format_name = AP4_GetFormatName(desc->GetFormat());
    if (format_name) {
        printf(" (%s)\n", format_name);
    } else {
        printf("\n");
    }
    if (desc->GetType() == AP4_SampleDescription::TYPE_MPEG) {
        // MPEG sample description
        AP4_MpegSampleDescription* mpeg_desc = AP4_DYNAMIC_CAST(AP4_MpegSampleDescription, desc);

        printf("    Stream Type: %s\n", mpeg_desc->GetStreamTypeString(mpeg_desc->GetStreamType()));
        printf("    Object Type: %s\n", mpeg_desc->GetObjectTypeString(mpeg_desc->GetObjectTypeId()));
        printf("    Max Bitrate: %d\n", mpeg_desc->GetMaxBitrate());
        printf("    Avg Bitrate: %d\n", mpeg_desc->GetAvgBitrate());
        printf("    Buffer Size: %d\n", mpeg_desc->GetBufferSize());
        
        if (mpeg_desc->GetObjectTypeId() == AP4_OTI_MPEG4_AUDIO          ||
            mpeg_desc->GetObjectTypeId() == AP4_OTI_MPEG2_AAC_AUDIO_LC   ||
            mpeg_desc->GetObjectTypeId() == AP4_OTI_MPEG2_AAC_AUDIO_MAIN) {
            AP4_MpegAudioSampleDescription* mpeg_audio_desc = AP4_DYNAMIC_CAST(AP4_MpegAudioSampleDescription, mpeg_desc);
            if (mpeg_audio_desc) ShowMpegAudioSampleDescription(*mpeg_audio_desc);
        }
    }
    AP4_AudioSampleDescription* audio_desc = 
        AP4_DYNAMIC_CAST(AP4_AudioSampleDescription, desc);
    if (audio_desc) {
        // Audio sample description
        printf("    Sample Rate: %d\n", audio_desc->GetSampleRate());
        printf("    Sample Size: %d\n", audio_desc->GetSampleSize());
        printf("    Channels:    %d\n", audio_desc->GetChannelCount());
    }
    AP4_VideoSampleDescription* video_desc = 
        AP4_DYNAMIC_CAST(AP4_VideoSampleDescription, desc);
    if (video_desc) {
        // Video sample description
        printf("    Width:       %d\n", video_desc->GetWidth());
        printf("    Height:      %d\n", video_desc->GetHeight());
        printf("    Depth:       %d\n", video_desc->GetDepth());
    }

    // Dolby Digital specifics
    if (desc->GetFormat() == AP4_SAMPLE_FORMAT_EC_3) {
        AP4_Dec3Atom* dec3 = AP4_DYNAMIC_CAST(AP4_Dec3Atom, desc->GetDetails().GetChild(AP4_ATOM_TYPE('d', 'e', 'c', '3')));
        if (dec3) {
            printf("    AC3 Data Rate: %d\n", dec3->GetDataRate());
            for (unsigned int i=0; i<dec3->GetSubStreams().ItemCount(); i++) {
                printf("    AC3 Substream %d:\n", i);
                printf("        fscod       = %d\n", dec3->GetSubStreams()[i].fscod);
                printf("        bsid        = %d\n", dec3->GetSubStreams()[i].bsid);
                printf("        bsmod       = %d\n", dec3->GetSubStreams()[i].bsmod);
                printf("        acmod       = %d\n", dec3->GetSubStreams()[i].acmod);
                printf("        lfeon       = %d\n", dec3->GetSubStreams()[i].lfeon);
                printf("        num_dep_sub = %d\n", dec3->GetSubStreams()[i].num_dep_sub);
                printf("        chan_loc    = %d\n", dec3->GetSubStreams()[i].chan_loc);
            }
            printf("    AC3 dec3 payload: [");
            ShowData(dec3->GetRawBytes());
            printf("]\n");
        }
    }
    
    // AVC specifics
    if (desc->GetType() == AP4_SampleDescription::TYPE_AVC) {
        // AVC Sample Description
        AP4_AvcSampleDescription* avc_desc = AP4_DYNAMIC_CAST(AP4_AvcSampleDescription, desc);
        const char* profile_name = AP4_AvccAtom::GetProfileName(avc_desc->GetProfile());
        printf("    AVC Profile:          %d", avc_desc->GetProfile());
        if (profile_name) {
            printf(" (%s)\n", profile_name);
        } else {
            printf("\n");
        }
        printf("    AVC Profile Compat:   %x\n", avc_desc->GetProfileCompatibility());
        printf("    AVC Level:            %d\n", avc_desc->GetLevel());
        printf("    AVC NALU Length Size: %d\n", avc_desc->GetNaluLengthSize());
        printf("    AVC SPS: [");
        const char* sep = "";
        for (unsigned int i=0; i<avc_desc->GetSequenceParameters().ItemCount(); i++) {
            printf("%s", sep);
            ShowData(avc_desc->GetSequenceParameters()[i]);
            sep = ", ";
        }
        printf("]\n");
        printf("    AVC PPS: [");
        sep = "";
        for (unsigned int i=0; i<avc_desc->GetPictureParameters().ItemCount(); i++) {
            printf("%s", sep);
            ShowData(avc_desc->GetPictureParameters()[i]);
            sep = ", ";
        }
        printf("]\n");
        printf("    Codecs String: ");
        AP4_String codec;
        avc_desc->GetCodecString(codec);
        printf("%s", codec.GetChars());
        printf("\n");
    } else if (desc->GetType() == AP4_SampleDescription::TYPE_HEVC) {
        // HEVC Sample Description
        AP4_HevcSampleDescription* hevc_desc = AP4_DYNAMIC_CAST(AP4_HevcSampleDescription, desc);
        const char* profile_name = AP4_HvccAtom::GetProfileName(hevc_desc->GetGeneralProfileSpace(), hevc_desc->GetGeneralProfile());
        printf("    HEVC Profile Space:       %d\n", hevc_desc->GetGeneralProfileSpace());
        printf("    HEVC Profile:             %d", hevc_desc->GetGeneralProfile());
        if (profile_name) printf(" (%s)", profile_name);
        printf("\n");
        printf("    HEVC Profile Compat:      %x\n", hevc_desc->GetGeneralProfileCompatibilityFlags());
        printf("    HEVC Level:               %d.%d\n", hevc_desc->GetGeneralLevel()/30, (hevc_desc->GetGeneralLevel()%30)/3);
        printf("    HEVC Tier:                %d\n", hevc_desc->GetGeneralTierFlag());
        printf("    HEVC Chroma Format:       %d", hevc_desc->GetChromaFormat());
        const char* chroma_format_name = AP4_HvccAtom::GetChromaFormatName(hevc_desc->GetChromaFormat());
        if (chroma_format_name) printf(" (%s)", chroma_format_name);
        printf("\n");
        printf("    HEVC Chroma Bit Depth:    %d\n", hevc_desc->GetChromaBitDepth());
        printf("    HEVC Luma Bit Depth:      %d\n", hevc_desc->GetLumaBitDepth());
        printf("    HEVC Average Frame Rate:  %d\n", hevc_desc->GetAverageFrameRate());
        printf("    HEVC Constant Frame Rate: %d\n", hevc_desc->GetConstantFrameRate());
        printf("    HEVC NALU Length Size:    %d\n", hevc_desc->GetNaluLengthSize());
        printf("    HEVC Sequences:\n");
        for (unsigned int i=0; i<hevc_desc->GetSequences().ItemCount(); i++) {
            const AP4_HvccAtom::Sequence& seq = hevc_desc->GetSequences()[i];
            printf("      {\n");
            printf("        Array Completeness=%d\n", seq.m_ArrayCompleteness);
            printf("        Type=%d", seq.m_NaluType);
            const char* nalu_type_name = AP4_HevcNalParser::NaluTypeName(seq.m_NaluType);
            if (nalu_type_name) {
                printf(" (%s)", nalu_type_name);
            }
            printf("\n");
            for (unsigned int j=0; j<seq.m_Nalus.ItemCount(); j++) {
                printf("        ");
                ShowData(seq.m_Nalus[j]);
            }
            printf("\n      }\n");
        }
        printf("    Codecs String: ");
        AP4_String codec;
        hevc_desc->GetCodecString(codec);
        printf("%s", codec.GetChars());
        printf("\n");
    }

    // Dolby Vision specifics
    AP4_DvccAtom* dvcc = AP4_DYNAMIC_CAST(AP4_DvccAtom, desc->GetDetails().GetChild(AP4_ATOM_TYPE_DVCC));
    if (dvcc) {
        printf("    Dolby Vision:\n");
        printf("      Version:     %d.%d\n", dvcc->GetDvVersionMajor(), dvcc->GetDvVersionMinor());
        const char* profile_name = AP4_DvccAtom::GetProfileName(dvcc->GetDvProfile());
        if (profile_name) {
            printf("      Profile:     %s\n", profile_name);
        } else {
            printf("      Profile:     %d\n", dvcc->GetDvProfile());
        }
        printf("      Level:       %d\n", dvcc->GetDvLevel());
        printf("      RPU Present: %s\n", dvcc->GetRpuPresentFlag()?"true":"false");
        printf("      EL Present:  %s\n", dvcc->GetElPresentFlag()?"true":"false");
        printf("      BL Present:  %s\n", dvcc->GetBlPresentFlag()?"true":"false");
    }
    
    // Subtitles
    if (desc->GetType() == AP4_SampleDescription::TYPE_SUBTITLES) {
        AP4_SubtitleSampleDescription* subt_desc = AP4_DYNAMIC_CAST(AP4_SubtitleSampleDescription, desc);
        printf("    Subtitles:\n");
        printf("       Namespace:       %s\n", subt_desc->GetNamespace().GetChars());
        printf("       Schema Location: %s\n", subt_desc->GetSchemaLocation().GetChars());
        printf("       Image Mime Type: %s\n", subt_desc->GetImageMimeType().GetChars());
    }
}

/*----------------------------------------------------------------------
|   ShowSampleDescription_Json
+---------------------------------------------------------------------*/
static void
ShowSampleDescription_Json(AP4_SampleDescription& description, bool verbose)
{
    printf("{\n");
    AP4_SampleDescription* desc = &description;
    if (desc->GetType() == AP4_SampleDescription::TYPE_PROTECTED) {
        AP4_ProtectedSampleDescription* prot_desc = AP4_DYNAMIC_CAST(AP4_ProtectedSampleDescription, desc);
        if (prot_desc) {
            ShowProtectedSampleDescription_Json(*prot_desc, verbose);
            printf(",\n");
            desc = prot_desc->GetOriginalSampleDescription();
        }
    }
    char coding[5];
    AP4_FormatFourChars(coding, desc->GetFormat());
    const char* format_name = AP4_GetFormatName(desc->GetFormat());
    printf("\"coding\":\"%s\",\n", coding);
    printf("\"coding_name\":");
    if (format_name) {
        printf("\"%s\"", format_name);
    } else {
        printf("\"\"");
    }
    
    if (desc->GetType() == AP4_SampleDescription::TYPE_MPEG) {
        // MPEG sample description
        AP4_MpegSampleDescription* mpeg_desc = AP4_DYNAMIC_CAST(AP4_MpegSampleDescription, desc);

        printf(",\n");
        printf("\"stream_type\":%d,\n",          mpeg_desc->GetStreamType());
        printf("\"stream_type_name\":\"%s\",\n", mpeg_desc->GetStreamTypeString(mpeg_desc->GetStreamType()));
        printf("\"object_type\":%d,\n",          mpeg_desc->GetObjectTypeId());
        printf("\"object_type_name\":\"%s\",\n", mpeg_desc->GetObjectTypeString(mpeg_desc->GetObjectTypeId()));
        printf("\"max_bitrate\":%d,\n",          mpeg_desc->GetMaxBitrate());
        printf("\"average_bitrate\":%d,\n",      mpeg_desc->GetAvgBitrate());
        printf("\"buffer_size\":%d,\n",             mpeg_desc->GetBufferSize());
        printf("\"decoder_info\": \"");
        ShowData(mpeg_desc->GetDecoderInfo());
        printf("\"");
        if (mpeg_desc->GetObjectTypeId() == AP4_OTI_MPEG4_AUDIO          ||
            mpeg_desc->GetObjectTypeId() == AP4_OTI_MPEG2_AAC_AUDIO_LC   ||
            mpeg_desc->GetObjectTypeId() == AP4_OTI_MPEG2_AAC_AUDIO_MAIN) {
            AP4_MpegAudioSampleDescription* mpeg_audio_desc = AP4_DYNAMIC_CAST(AP4_MpegAudioSampleDescription, mpeg_desc);
            if (mpeg_audio_desc) {
                printf(",\n");
                ShowMpegAudioSampleDescription(*mpeg_audio_desc);
            }
        }
    }
    AP4_AudioSampleDescription* audio_desc = AP4_DYNAMIC_CAST(AP4_AudioSampleDescription, desc);
    if (audio_desc) {
        // Audio sample description
        printf(",\n");
        printf("\"sample_rate\":%d,\n", audio_desc->GetSampleRate());
        printf("\"sample_size\":%d,\n", audio_desc->GetSampleSize());
        printf("\"channels\":%d",       audio_desc->GetChannelCount());
    }
    AP4_VideoSampleDescription* video_desc = 
        AP4_DYNAMIC_CAST(AP4_VideoSampleDescription, desc);
    if (video_desc) {
        // Video sample description
        printf(",\n");
        printf("\"width\":%d,\n",  video_desc->GetWidth());
        printf("\"height\":%d,\n", video_desc->GetHeight());
        printf("\"depth\":%d",     video_desc->GetDepth());
    }

    // Dolby Digital specifics
    if (desc->GetFormat() == AP4_SAMPLE_FORMAT_EC_3) {
        AP4_Dec3Atom* dec3 = AP4_DYNAMIC_CAST(AP4_Dec3Atom, desc->GetDetails().GetChild(AP4_ATOM_TYPE('d', 'e', 'c', '3')));
        if (dec3) {
            printf(",\n");
            printf("\"dolby_digital_info\": {\n");
            printf("  \"dec3_payload\": \"");
            ShowData(dec3->GetRawBytes());
            printf("\",\n");
            printf("  \"data_rate\": %d,\n", dec3->GetDataRate());
            printf("  \"substreams\": [\n");
            const char* sep = "";
            for (unsigned int i=0; i<dec3->GetSubStreams().ItemCount(); i++) {
                printf("%s    {\n", sep);
                printf("      \"fscod\": %d,\n", dec3->GetSubStreams()[i].fscod);
                printf("      \"bsid\": %d,\n",  dec3->GetSubStreams()[i].bsid);
                printf("      \"bsmod\": %d,\n", dec3->GetSubStreams()[i].bsmod);
                printf("      \"acmod\": %d,\n", dec3->GetSubStreams()[i].acmod);
                printf("      \"lfeon\": %d,\n", dec3->GetSubStreams()[i].lfeon);
                printf("      \"num_dep_sub\": %d,\n", dec3->GetSubStreams()[i].num_dep_sub);
                printf("      \"chan_loc\": %d\n", dec3->GetSubStreams()[i].chan_loc);
                printf("    }");
                sep = ",\n";
            }
            printf("\n  ]\n}");
        }
    }

    // AVC specifics
    if (desc->GetType() == AP4_SampleDescription::TYPE_AVC) {
        // AVC Sample Description
        AP4_AvcSampleDescription* avc_desc = AP4_DYNAMIC_CAST(AP4_AvcSampleDescription, desc);
        printf(",\n");
        printf("\"avc_profile\":%d,\n",        avc_desc->GetProfile());
        const char* profile_name = AP4_AvccAtom::GetProfileName(avc_desc->GetProfile());
        if (profile_name) printf("\"avc_profile_name\":\"%s\",\n", profile_name);
        printf("\"avc_profile_compat\":%d,\n", avc_desc->GetProfileCompatibility());
        printf("\"avc_level\":%d,\n",          avc_desc->GetLevel());
        printf("\"avc_nalu_length_size\":%d,\n",  avc_desc->GetNaluLengthSize());
        printf("\"avc_sps\": [");
        const char* sep = "";
        for (unsigned int i=0; i<avc_desc->GetSequenceParameters().ItemCount(); i++) {
            printf("%s", sep);
            printf("\"");
            ShowData(avc_desc->GetSequenceParameters()[i]);
            printf("\"");
            sep = ", ";
        }
        printf("],\n");
        printf("\"avc_pps\": [");
        sep = "";
        for (unsigned int i=0; i<avc_desc->GetPictureParameters().ItemCount(); i++) {
            printf("%s", sep);
            printf("\"");
            ShowData(avc_desc->GetPictureParameters()[i]);
            printf("\"");
            sep = ", ";
        }
        printf("],\n");
        printf("\"codecs_string\":\"");
        AP4_String codec;
        avc_desc->GetCodecString(codec);
        printf("%s", codec.GetChars());
        printf("\"");
    } else if (desc->GetType() == AP4_SampleDescription::TYPE_HEVC) {
        // HEVC Sample Description
        AP4_HevcSampleDescription* hevc_desc = AP4_DYNAMIC_CAST(AP4_HevcSampleDescription, desc);
        printf(",\n");
        printf("\"hevc_profile_space\":%d,\n", hevc_desc->GetGeneralProfileSpace());
        printf("\"hevc_profile\":%d,\n", hevc_desc->GetGeneralProfile());
        const char* profile_name = AP4_HvccAtom::GetProfileName(hevc_desc->GetGeneralProfileSpace(), hevc_desc->GetGeneralProfile());
        if (profile_name) printf("\"hevc_profile_name\":\"%s\",\n", profile_name);
        printf("\"hevc_profile_compat\":%d,\n", hevc_desc->GetGeneralProfileCompatibilityFlags());
        printf("\"hevc_constraints\":%llu,\n", hevc_desc->GetGeneralConstraintIndicatorFlags());
        printf("\"hevc_level\":%d,\n", hevc_desc->GetGeneralLevel());
        printf("\"hevc_level_name\":\"%d.%d\",\n", hevc_desc->GetGeneralLevel()/30, (hevc_desc->GetGeneralLevel()%30)/3);
        printf("\"hevc_tier\":%d,\n", hevc_desc->GetGeneralTierFlag());
        printf("\"hevc_chroma_format\":%d,\n", hevc_desc->GetChromaFormat());
        const char* chroma_format_name = AP4_HvccAtom::GetChromaFormatName(hevc_desc->GetChromaFormat());
        if (chroma_format_name) printf("\"hevc_chroma_format_name\":\"%s\",\n", chroma_format_name);
        printf("\"hevc_chroma_bit_depth\":%d,\n", hevc_desc->GetChromaBitDepth());
        printf("\"hevc_lunma_bit_depth\":%d,\n", hevc_desc->GetLumaBitDepth());
        printf("\"hevc_average_frame_rate\":%d,\n", hevc_desc->GetAverageFrameRate());
        printf("\"hevc_constant_frame_rate\":%d,\n", hevc_desc->GetConstantFrameRate());
        printf("\"hevc_nalu_length_size\":%d,\n", hevc_desc->GetNaluLengthSize());
        printf("\"hevc_sequences\": [\n");
        const char* seq_sep = "";
        for (unsigned int i=0; i<hevc_desc->GetSequences().ItemCount(); i++) {
            const AP4_HvccAtom::Sequence& seq = hevc_desc->GetSequences()[i];
            printf("%s      {\n", seq_sep);
            printf("        \"array_completeness\":%d,\n", seq.m_ArrayCompleteness);
            printf("        \"type\":%d,\n", seq.m_NaluType);
            const char* nalu_type_name = AP4_HevcNalParser::NaluTypeName(seq.m_NaluType);
            if (nalu_type_name) {
                printf("        \"type_name\":\"%s\",\n", nalu_type_name);
            }
            printf("        \"data\":[");
            const char* sep = "";
            for (unsigned int j=0; j<seq.m_Nalus.ItemCount(); j++) {
                printf("%s", sep);
                printf("\"");
                ShowData(seq.m_Nalus[j]);
                printf("\"");
                sep = ", ";
            }
            printf("]\n      }");
            seq_sep = ",\n";
        }
        printf("\n],\n");
        printf("\"codecs_string\":\"");
        AP4_String codec;
        hevc_desc->GetCodecString(codec);
        printf("%s", codec.GetChars());
        printf("\"");
    }

    // Dolby Vision specifics
    AP4_DvccAtom* dvcc = AP4_DYNAMIC_CAST(AP4_DvccAtom, desc->GetDetails().GetChild(AP4_ATOM_TYPE_DVCC));
    if (dvcc) {
        printf(",\n");
        printf("\"dolby_vision\": {\n");
        printf("   \"version_major\": %d,\n", dvcc->GetDvVersionMajor());
        printf("   \"version_minor\": %d,\n", dvcc->GetDvVersionMinor());
        printf("   \"profile\": %d,\n", dvcc->GetDvProfile());
        const char* profile_name = AP4_DvccAtom::GetProfileName(dvcc->GetDvProfile());
        printf("   \"profile_name\": \"%s\",\n", profile_name?profile_name:"");
        printf("   \"level\": %d,\n", dvcc->GetDvLevel());
        printf("   \"rpu_present\": %s,\n", dvcc->GetRpuPresentFlag()?"true":"false");
        printf("   \"el_present\": %s,\n", dvcc->GetElPresentFlag()?"true":"false");
        printf("   \"bl_present\": %s\n", dvcc->GetBlPresentFlag()?"true":"false");
        printf("}");
    }
    
    // Subtitles
    if (desc->GetType() == AP4_SampleDescription::TYPE_SUBTITLES) {
        printf(",\n");
        printf("\"subtitles\": {\n");
        AP4_SubtitleSampleDescription* subt_desc = AP4_DYNAMIC_CAST(AP4_SubtitleSampleDescription, desc);
        printf("  \"namespace\": \"%s\",\n",       subt_desc->GetNamespace().GetChars());
        printf("  \"schema_location\": \"%s\",\n", subt_desc->GetSchemaLocation().GetChars());
        printf("  \"image_mime_type\": \"%s\"\n",  subt_desc->GetImageMimeType().GetChars());
        printf("}");
    }
    
    printf("\n}");
}

/*----------------------------------------------------------------------
|   ShowSampleDescription
+---------------------------------------------------------------------*/
static void
ShowSampleDescription(AP4_SampleDescription& description, bool verbose)
{
    switch (Options.format) {
        case TEXT_FORMAT: ShowSampleDescription_Text(description, verbose); break;
        case JSON_FORMAT: ShowSampleDescription_Json(description, verbose); break;
    }
}

/*----------------------------------------------------------------------
|   ShowDcfInfo
+---------------------------------------------------------------------*/
static void
ShowDcfInfo(AP4_File& file)
{
    AP4_FtypAtom* ftyp = file.GetFileType();
    if (ftyp == NULL) return;
    printf("OMA DCF File, version=%d\n", ftyp->GetMinorVersion());
    if (ftyp->GetMinorVersion() != 2) return;

    AP4_OdheAtom* odhe = AP4_DYNAMIC_CAST(AP4_OdheAtom, file.FindChild("odrm/odhe"));
    if (odhe) {
        printf("Content Type:      %s\n", odhe->GetContentType().GetChars());
    }
    AP4_OhdrAtom* ohdr = AP4_DYNAMIC_CAST(AP4_OhdrAtom, file.FindChild("odrm/odhe/ohdr"));
    if (ohdr) {
        printf("Encryption Method: ");
        switch (ohdr->GetEncryptionMethod()) {
            case AP4_OMA_DCF_ENCRYPTION_METHOD_NULL:    printf("NULL\n");        break;
            case AP4_OMA_DCF_ENCRYPTION_METHOD_AES_CBC: printf("AES-128-CBC\n"); break;
            case AP4_OMA_DCF_ENCRYPTION_METHOD_AES_CTR: printf("AES-128-CTR\n"); break;
            default:                                    printf("%d\n", ohdr->GetEncryptionMethod()); 
        }
        printf("Padding Scheme:    ");
        switch (ohdr->GetPaddingScheme()) {
            case AP4_OMA_DCF_PADDING_SCHEME_NONE:     printf("NONE\n"); break;
            case AP4_OMA_DCF_PADDING_SCHEME_RFC_2630: printf("RFC 2630\n"); break;
            default:                                  printf("%d\n", ohdr->GetPaddingScheme());
        }
        printf("Content ID:        %s\n", ohdr->GetContentId().GetChars());
        printf("Rights Issuer URL: %s\n", ohdr->GetRightsIssuerUrl().GetChars());
        printf("Textual Headers:\n");
        
        AP4_Size    headers_size = ohdr->GetTextualHeaders().GetDataSize();
        const char* headers = (const char*)ohdr->GetTextualHeaders().GetData();
        while (headers_size) {
            printf("  %s\n", headers);
            AP4_Size header_len = (AP4_Size)strlen(headers);
            headers_size -= header_len+1;
            headers      += header_len+1;
        }
        AP4_GrpiAtom* grpi = AP4_DYNAMIC_CAST(AP4_GrpiAtom, ohdr->GetChild(AP4_ATOM_TYPE_GRPI));
        if (grpi) {
            printf("Group ID:          %s\n", grpi->GetGroupId().GetChars());
        }
    }
}

/*----------------------------------------------------------------------
|   ReadGolomb
+---------------------------------------------------------------------*/
static unsigned int
ReadGolomb(AP4_BitStream& bits)
{
    unsigned int leading_zeros = 0;
    while (bits.ReadBit() == 0) {
        leading_zeros++;
    }
    if (leading_zeros) {
        return (1<<leading_zeros)-1+bits.ReadBits(leading_zeros);
    } else {
        return 0;
    }
}

/*----------------------------------------------------------------------
|   ShowAvcInfo
+---------------------------------------------------------------------*/
static void
ShowAvcInfo(const AP4_DataBuffer& sample_data, AP4_AvcSampleDescription* avc_desc) 
{
    const unsigned char* data = sample_data.GetData();
    AP4_Size             size = sample_data.GetDataSize();

    while (size >= avc_desc->GetNaluLengthSize()) {
        unsigned int nalu_length = 0;
        if (avc_desc->GetNaluLengthSize() == 1) {
            nalu_length = *data++;
            --size;
        } else if (avc_desc->GetNaluLengthSize() == 2) {
            nalu_length = AP4_BytesToUInt16BE(data);
            data += 2;
            size -= 2;
        } else if (avc_desc->GetNaluLengthSize() == 4) {
            nalu_length = AP4_BytesToUInt32BE(data);
            data += 4;
            size -= 4;
        } else {
            return;
        }
        if (nalu_length <= size) {
            size -= nalu_length;
        } else {
            size = 0;
        }
        
        switch (*data & 0x1F) {
            case 1: {
                AP4_BitStream bits;
                bits.WriteBytes(data+1, 8);
                ReadGolomb(bits);
                unsigned int slice_type = ReadGolomb(bits);
                switch (slice_type) {
                    case 0: printf("<P>");  break;
                    case 1: printf("<B>");  break;
                    case 2: printf("<I>");  break;
                    case 3:	printf("<SP>"); break;
                    case 4: printf("<SI>"); break;
                    case 5: printf("<P>");  break;
                    case 6: printf("<B>");  break;
                    case 7: printf("<I>");  break;
                    case 8:	printf("<SP>"); break;
                    case 9: printf("<SI>"); break;
                    default: printf("<S/%d>", slice_type); break;
                }
                return; // only show first slice type
            }
            
            case 5: 
                printf("<I>"); 
                return;
        }
        
        data += nalu_length;
    }
}

/*----------------------------------------------------------------------
|   ShowSample_Text
+---------------------------------------------------------------------*/
static void
ShowSample_Text(AP4_Track&      track,
                AP4_Sample&     sample,
                AP4_DataBuffer& sample_data,
                unsigned int    index,
                bool            verbose,
                bool            show_sample_data,
                AP4_AvcSampleDescription* avc_desc)
{
    printf("[%06d] size=%6d duration=%6d",
           index+1, 
           (int)sample.GetSize(), 
           (int)sample.GetDuration());
    if (verbose) {
        printf(" (%6d ms) offset=%10lld dts=%10lld (%10lld ms) cts=%10lld (%10lld ms) [%d]", 
               (int)AP4_ConvertTime(sample.GetDuration(), track.GetMediaTimeScale(), 1000),
               sample.GetOffset(),
               sample.GetDts(), 
               AP4_ConvertTime(sample.GetDts(), track.GetMediaTimeScale(), 1000),
               sample.GetCts(),
               AP4_ConvertTime(sample.GetCts(), track.GetMediaTimeScale(), 1000),
               sample.GetDescriptionIndex());
    }
    if (sample.IsSync()) {
        printf(" [S] ");
    } else {
        printf("     ");
    }
    if (avc_desc || show_sample_data) {
        sample.ReadData(sample_data);
    }
    if (avc_desc) {
        ShowAvcInfo(sample_data, avc_desc);
    }
    if (show_sample_data) {
        unsigned int show = sample_data.GetDataSize();
        if (!verbose) {
            if (show > 12) show = 12; // max first 12 chars
        }
        
        for (unsigned int i=0; i<show; i++) {
            if (verbose) {
                if (i%16 == 0) {
                    printf("\n%06d: ", i);
                }
            }
            printf("%02x", sample_data.GetData()[i]);
            if (verbose) printf(" ");
        }
        if (show != sample_data.GetDataSize()) {
            printf("...");
        }
    }
}

/*----------------------------------------------------------------------
|   ScanMedia
+---------------------------------------------------------------------*/
static void
ScanMedia(AP4_Movie& movie, AP4_Track& track, AP4_ByteStream& stream, MediaInfo& info)
{
    AP4_UI64 total_size = 0;
    AP4_UI64 total_duration = 0;
    
    AP4_UI64 position;
    stream.Tell(position);
    stream.Seek(0);
    AP4_LinearReader reader(movie, &stream);
    reader.EnableTrack(track.GetId());
    
    info.sample_count = 0;
    
    AP4_Sample sample;
    if (movie.HasFragments()) {
        AP4_DataBuffer sample_data;
        for(unsigned int i=0; ; i++) {
            AP4_UI32 track_id = 0;
            AP4_Result result = reader.ReadNextSample(sample, sample_data, track_id);
            if (AP4_SUCCEEDED(result)) {
                total_size += sample.GetSize();
                total_duration += sample.GetDuration();
                ++info.sample_count;
            } else {
                break;
            }
        }
    } else {
        info.sample_count = track.GetSampleCount();
        for (unsigned int i=0; i<track.GetSampleCount(); i++) {
            if (AP4_SUCCEEDED(track.GetSample(i, sample))) {
                total_size += sample.GetSize();
            }
        }
        total_duration = track.GetMediaDuration();
    }
    info.duration = total_duration;
    
    double duration_ms = (double)AP4_ConvertTime(total_duration, track.GetMediaTimeScale(), 1000);
    if (duration_ms) {
        info.bitrate = 8.0*1000.0*(double)total_size/duration_ms;
    } else {
        info.bitrate = 0.0;
    }
}

/*----------------------------------------------------------------------
|   ShowTrackInfo_Text
+---------------------------------------------------------------------*/
static void
ShowTrackInfo_Text(AP4_Movie& movie, AP4_Track& track, AP4_ByteStream& stream, bool show_samples, bool show_sample_data, bool verbose, bool fast)
{
    printf("  flags:        %d", track.GetFlags());
    if (track.GetFlags() & AP4_TRACK_FLAG_ENABLED) {
        printf(" ENABLED");
    }
    if (track.GetFlags() & AP4_TRACK_FLAG_IN_MOVIE) {
        printf(" IN-MOVIE");
    }
    if (track.GetFlags() & AP4_TRACK_FLAG_IN_PREVIEW) {
        printf(" IN-PREVIEW");
    }
    printf("\n");
	printf("  id:           %d\n", track.GetId());
    printf("  type:         ");
    switch (track.GetType()) {
        case AP4_Track::TYPE_AUDIO:     printf("Audio\n");     break;
        case AP4_Track::TYPE_VIDEO:     printf("Video\n");     break;
        case AP4_Track::TYPE_HINT:      printf("Hint\n");      break;
        case AP4_Track::TYPE_SYSTEM:    printf("System\n");    break;
        case AP4_Track::TYPE_TEXT:      printf("Text\n");      break;
        case AP4_Track::TYPE_JPEG:      printf("JPEG\n");      break;
        case AP4_Track::TYPE_SUBTITLES: printf("Subtitles\n"); break;
        default: {
            char hdlr[5];
            AP4_FormatFourChars(hdlr, track.GetHandlerType());
            printf("Unknown [");
            printf("%s", hdlr);
            printf("]\n");
            break;
        }
    }
    printf("  duration: %d ms\n", track.GetDurationMs());
    printf("  language: %s\n", track.GetTrackLanguage());
    printf("  media:\n");
    printf("    sample count: %d\n", track.GetSampleCount());
    printf("    timescale:    %d\n", track.GetMediaTimeScale());
    printf("    duration:     %lld (media timescale units)\n", track.GetMediaDuration());
    printf("    duration:     %d (ms)\n", (AP4_UI32)AP4_ConvertTime(track.GetMediaDuration(), track.GetMediaTimeScale(), 1000));
    if (!fast) {
        MediaInfo media_info;
        ScanMedia(movie, track, stream, media_info);
        printf("    bitrate (computed): %.3f Kbps\n", media_info.bitrate/1000.0);
        if (movie.HasFragments()) {
            printf("    sample count with fragments: %lld\n", media_info.sample_count);
            printf("    duration with fragments:     %lld\n", media_info.duration);
            printf("    duration with fragments:     %d (ms)\n", (AP4_UI32)AP4_ConvertTime(media_info.duration, track.GetMediaTimeScale(), 1000));
        }
    }
    if (track.GetWidth()  || track.GetHeight()) {
        printf("  display width:  %f\n", (float)track.GetWidth()/65536.0);
        printf("  display height: %f\n", (float)track.GetHeight()/65536.0);
    }
    if (track.GetType() == AP4_Track::TYPE_VIDEO && track.GetSampleCount()) {
        printf("  frame rate (computed): %.3f\n", (float)track.GetSampleCount()/
                                                  ((float)track.GetMediaDuration()/(float)track.GetMediaTimeScale()));
    }
    
    // show all sample descriptions
    AP4_AvcSampleDescription* avc_desc = NULL;
    for (unsigned int desc_index=0;
        AP4_SampleDescription* sample_desc = track.GetSampleDescription(desc_index);
        desc_index++) {
        printf("  Sample Description %d\n", desc_index);
        ShowSampleDescription(*sample_desc, verbose);
        avc_desc = AP4_DYNAMIC_CAST(AP4_AvcSampleDescription, sample_desc);
    }

    // show samples if requested
    if (show_samples) {
        AP4_Sample     sample;
        AP4_DataBuffer sample_data;
        AP4_Ordinal    index = 0;
        while (AP4_SUCCEEDED(track.GetSample(index, sample))) {
            if (avc_desc || show_sample_data) {
                sample.ReadData(sample_data);
            }

            ShowSample_Text(track, sample, sample_data, index, verbose, show_sample_data, avc_desc);
            printf("\n");
            index++;
        }
    }
}

/*----------------------------------------------------------------------
|   ShowTrackInfo_Json
+---------------------------------------------------------------------*/
static void
ShowTrackInfo_Json(AP4_Movie& movie, AP4_Track& track, AP4_ByteStream& stream, bool /*show_samples*/, bool /*show_sample_data*/, bool verbose, bool fast)
{
    printf("{\n");
    printf("  \"flags\":%d,\n", track.GetFlags());
    printf("  \"flag_names\":[");
    const char* sep = "";
    if (track.GetFlags() & AP4_TRACK_FLAG_ENABLED) {
        printf("\"ENABLED\"");
        sep = " ,";
    }
    if (track.GetFlags() & AP4_TRACK_FLAG_IN_MOVIE) {
        printf("%s\"IN-MOVIE\"", sep);
        sep = " ,";
    }
    if (track.GetFlags() & AP4_TRACK_FLAG_IN_PREVIEW) {
        printf("%s\"IN-PREVIEW\"", sep);
    }
    printf("],\n");
	printf("  \"id\":%d,\n", track.GetId());
    printf("  \"type\":");
    switch (track.GetType()) {
        case AP4_Track::TYPE_AUDIO:     printf("\"Audio\"");     break;
        case AP4_Track::TYPE_VIDEO:     printf("\"Video\"");     break;
        case AP4_Track::TYPE_HINT:      printf("\"Hint\"");      break;
        case AP4_Track::TYPE_SYSTEM:    printf("\"System\"");    break;
        case AP4_Track::TYPE_TEXT:      printf("\"Text\"");      break;
        case AP4_Track::TYPE_JPEG:      printf("\"JPEG\"");      break;
        case AP4_Track::TYPE_SUBTITLES: printf("\"Subtitles\""); break;
        default: {
            char hdlr[5];
            AP4_FormatFourChars(hdlr, track.GetHandlerType());
            printf("\"Unknown [");
            printf("%s", hdlr);
            printf("]\"");
            break;
        }
    }
    printf(",\n");
    printf("  \"duration_ms\":%d,\n", track.GetDurationMs());
    printf("  \"language\":\"%s\",\n", track.GetTrackLanguage());
    printf("  \"media\":{\n");
    printf("    \"sample_count\":%d,\n", track.GetSampleCount());
    printf("    \"timescale\":%d,\n", track.GetMediaTimeScale());
    printf("    \"duration\":%lld,\n", track.GetMediaDuration());
    printf("    \"duration_ms\":%d", (AP4_UI32)AP4_ConvertTime(track.GetMediaDuration(), track.GetMediaTimeScale(), 1000));
    if (!fast) {
        MediaInfo media_info;
        ScanMedia(movie, track, stream, media_info);
        printf(",\n");
        printf("    \"bitrate\":%.3f", media_info.bitrate/1000.0);
        if (movie.HasFragments()) {
            printf(",\n");
            printf("    \"sample_count_with_fragments\":%lld,\n", media_info.sample_count);
            printf("    \"duration_with_fragments\":%lld,\n", media_info.duration);
            printf("    \"duration_with_fragments_ms\":%d", (AP4_UI32)AP4_ConvertTime(media_info.duration, track.GetMediaTimeScale(), 1000));
        }
        printf("\n");
    } else {
        printf("\n");
    }
    printf("  },\n");
    if (track.GetWidth()  || track.GetHeight()) {
        printf("  \"display_width\":%f,\n", (float)track.GetWidth()/65536.0);
        printf("  \"display_height\":%f,\n", (float)track.GetHeight()/65536.0);
    }
    if (track.GetType() == AP4_Track::TYPE_VIDEO && track.GetSampleCount()) {
        printf("  \"frame_rate\":%.3f,\n", (float)1000*track.GetSampleCount()/
                                                  (float)track.GetDurationMs());
    }
    
    // show all sample descriptions
    printf("  \"sample_descriptions\":[\n");
    //AP4_AvcSampleDescription* avc_desc = NULL;
    sep = "";
    for (unsigned int desc_index=0;
        AP4_SampleDescription* sample_desc = track.GetSampleDescription(desc_index);
        desc_index++) {
        printf("%s", sep);
        ShowSampleDescription(*sample_desc, verbose);
        sep = ",\n";
        //if (sample_desc->GetFormat() == AP4_SAMPLE_FORMAT_AVC1) {
        //    avc_desc = AP4_DYNAMIC_CAST(AP4_AvcSampleDescription, sample_desc);
        //}
    }
    printf("  ]\n");
    printf("}");
}

/*----------------------------------------------------------------------
|   ShowTrackInfo
+---------------------------------------------------------------------*/
static void
ShowTrackInfo(AP4_Movie& movie, AP4_Track& track, AP4_ByteStream& stream, bool show_samples, bool show_sample_data, bool verbose, bool fast)
{
    switch (Options.format) {
        case TEXT_FORMAT: 
            ShowTrackInfo_Text(movie, track, stream, show_samples, show_sample_data, verbose, fast);
            break;

        case JSON_FORMAT: 
            ShowTrackInfo_Json(movie, track, stream, show_samples, show_sample_data, verbose, fast);
            break;
    }
}

/*----------------------------------------------------------------------
|   ShowMovieInfo
+---------------------------------------------------------------------*/
static void
ShowMovieInfo(AP4_Movie& movie)
{
    switch (Options.format) {
        case TEXT_FORMAT:
            printf("Movie:\n");
            printf("  duration:   %d ms\n", movie.GetDurationMs());
            printf("  time scale: %d\n", movie.GetTimeScale());
            printf("  fragments:  %s\n", movie.HasFragments()?"yes":"no");
            printf("\n");
            break;
            
        case JSON_FORMAT:
            printf("\"movie\":{\n");
            printf("  \"duration_ms\":%d,\n", movie.GetDurationMs());
            printf("  \"duration\":%lld,\n", movie.GetDuration());
            printf("  \"time_scale\":%d,\n", movie.GetTimeScale());
            printf("  \"fragments\":%s\n", movie.HasFragments()?"true":"false");
            printf("},\n");
    }
}

/*----------------------------------------------------------------------
|   ShowFileInfo
+---------------------------------------------------------------------*/
static void
ShowFileInfo(AP4_File& file)
{
    AP4_FtypAtom* file_type = file.GetFileType();
    if (file_type == NULL) return;
    char four_cc[5];

    AP4_FormatFourChars(four_cc, file_type->GetMajorBrand());
    switch (Options.format) {
        case TEXT_FORMAT:
            printf("File:\n");
            printf("  major brand:      %s\n", four_cc);
            printf("  minor version:    %x\n", file_type->GetMinorVersion());
            break;
            
        case JSON_FORMAT:
            printf("\"file\":{\n");
            printf("  \"major_brand\":\"%s\",\n", four_cc);
            printf("  \"minor_version\":%d,\n", file_type->GetMinorVersion());
            break;
    }
    
    // compatible brands
    if (Options.format == JSON_FORMAT) printf("  \"compatible_brands\":[");
    const char* sep = "";
    for (unsigned int i=0; i<file_type->GetCompatibleBrands().ItemCount(); i++) {
        AP4_UI32 cb = file_type->GetCompatibleBrands()[i];
        if (cb == 0) continue;
        AP4_FormatFourChars(four_cc, cb);
        switch (Options.format) {
            case TEXT_FORMAT:
                printf("  compatible brand: %s\n", four_cc);
                break;
                
            case JSON_FORMAT:
                printf("%s\"%s\"", sep, four_cc);
                sep = ", ";
                break;
        }
    }
    
    if (Options.format == JSON_FORMAT) {
        printf("],\n");
    }

    // fast-start
    switch (Options.format) {
        case TEXT_FORMAT:
            printf("  fast start:       %s\n\n", file.IsMoovBeforeMdat() ? "yes" : "no");
            break;
            
        case JSON_FORMAT:
            printf("  \"fast_start\":%s\n},\n", file.IsMoovBeforeMdat() ? "true" : "false");
            break;
    }
}


/*----------------------------------------------------------------------
|   ShowTracks
+---------------------------------------------------------------------*/
static void
ShowTracks(AP4_Movie& movie, AP4_List<AP4_Track>& tracks, AP4_ByteStream& stream, bool show_samples, bool show_sample_data, bool verbose, bool fast)
{
    if (Options.format == JSON_FORMAT) printf("\"tracks\":[\n");
    int index=1;
    for (AP4_List<AP4_Track>::Item* track_item = tracks.FirstItem();
         track_item;
         track_item = track_item->GetNext(), ++index) {
        if (Options.format == TEXT_FORMAT) {
            printf("Track %d:\n", index); 
        }
        if (Options.format == JSON_FORMAT && index > 1) printf(",\n"); 
        ShowTrackInfo(movie, *track_item->GetData(), stream, show_samples, show_sample_data, verbose, fast);
    }
    if (Options.format == JSON_FORMAT) printf("]\n");
}

/*----------------------------------------------------------------------
|   ShowMarlinTracks
+---------------------------------------------------------------------*/
static void
ShowMarlinTracks(AP4_File& file, AP4_ByteStream& stream, AP4_List<AP4_Track>& tracks, bool show_samples, bool show_sample_data, bool verbose, bool fast)
{
    if (Options.format != TEXT_FORMAT) return;
    
    AP4_List<AP4_MarlinIpmpParser::SinfEntry> sinf_entries;
    AP4_Result result = AP4_MarlinIpmpParser::Parse(file, stream, sinf_entries);
    if (AP4_FAILED(result)) {
        printf("WARNING: cannot parse Marlin IPMP info\n");
        ShowTracks(*file.GetMovie(), tracks, stream, show_samples, show_sample_data, verbose, fast);
        return;
    }
    int index=1;
    for (AP4_List<AP4_Track>::Item* track_item = tracks.FirstItem();
         track_item;
         track_item = track_item->GetNext(), ++index) {
        printf("Track %d:\n", index); 
        AP4_Track* track = track_item->GetData();
        ShowTrackInfo(*file.GetMovie(), *track, stream, show_samples, show_sample_data, verbose, fast);
        
        for (AP4_List<AP4_MarlinIpmpParser::SinfEntry>::Item* sinf_entry_item = sinf_entries.FirstItem();
             sinf_entry_item;
             sinf_entry_item = sinf_entry_item->GetNext()) {
             AP4_MarlinIpmpParser::SinfEntry* sinf_entry = sinf_entry_item->GetData();
            if (sinf_entry->m_TrackId == track->GetId()) {
                printf("    [ENCRYPTED]\n");
                if (sinf_entry->m_Sinf == NULL) {
                    printf("WARNING: NULL sinf entry for track ID %d\n", track->GetId());
                } else {
                    AP4_ContainerAtom* schi = AP4_DYNAMIC_CAST(AP4_ContainerAtom, sinf_entry->m_Sinf->GetChild(AP4_ATOM_TYPE_SCHI));
                    AP4_SchmAtom*      schm = AP4_DYNAMIC_CAST(AP4_SchmAtom, sinf_entry->m_Sinf->GetChild(AP4_ATOM_TYPE_SCHM));
                    if (schm && schi) {
                        AP4_UI32 scheme_type = schm->GetSchemeType();
                        printf("      Scheme Type:    %c%c%c%c\n", 
                            (char)((scheme_type>>24) & 0xFF),
                            (char)((scheme_type>>16) & 0xFF),
                            (char)((scheme_type>> 8) & 0xFF),
                            (char)((scheme_type    ) & 0xFF));
                        printf("      Scheme Version: %d\n", schm->GetSchemeVersion());
                        ShowProtectionSchemeInfo_Text(scheme_type, *schi, verbose);
                    } else {
                        if (schm == NULL) printf("WARNING: schm atom is NULL\n");
                        if (schi == NULL) printf("WARNING: schi atom is NULL\n");
                    }
                }
            }
        }
    }
    sinf_entries.DeleteReferences();
}

/*----------------------------------------------------------------------
|   ShowSampleLayout
+---------------------------------------------------------------------*/
static void
ShowSampleLayout(AP4_List<AP4_Track>& tracks, bool /* verbose */)
{
    AP4_Array<int> cursors;
    cursors.SetItemCount(tracks.ItemCount());
    for (unsigned int i=0; i<tracks.ItemCount(); i++) {
        cursors[i] = 0;
    }
    
    AP4_Sample  sample;
    AP4_UI64    sample_offset  = 0;
    AP4_UI32    sample_size    = 0;
    AP4_UI64    sample_dts     = 0;
    bool        sample_is_sync = false;
    bool        indicator      = true;
    AP4_Track*  previous_track = NULL;
    for (unsigned int i=0;;i++) {
        AP4_UI64    min_offset = (AP4_UI64)(-1);
        int         chosen_index = -1;
        AP4_Track*  chosen_track = NULL;
        AP4_Ordinal index = 0;
        for (AP4_List<AP4_Track>::Item* track_item = tracks.FirstItem();
             track_item;
             track_item = track_item->GetNext()) {
             AP4_Track* track = track_item->GetData();
             AP4_Result result = track->GetSample(cursors[index], sample);
             if (AP4_SUCCEEDED(result)) {
                if (sample.GetOffset() < min_offset) {
                    chosen_index   = index;
                    chosen_track   = track;
                    sample_offset  = sample.GetOffset();
                    sample_size    = sample.GetSize();
                    sample_dts     = sample.GetDts();
                    sample_is_sync = sample.IsSync();
                    min_offset     = sample_offset;
                }
             }
             index++;
        }
        
        // stop if we've exhausted all samples
        if (chosen_index == -1) break;
        
        // update the cursor for the chosen track
        cursors[chosen_index] = cursors[chosen_index]+1;

        // see if we've changed tracks
        if (previous_track != chosen_track) {
            previous_track = chosen_track;
            indicator = !indicator;
        }
        
        // show the chosen track/sample
        char track_type = ' ';
        switch (chosen_track->GetType()) {
            case AP4_Track::TYPE_AUDIO:     track_type = 'A'; break;
            case AP4_Track::TYPE_VIDEO:     track_type = 'V'; break;
            case AP4_Track::TYPE_HINT:      track_type = 'H'; break;
            case AP4_Track::TYPE_TEXT:      track_type = 'T'; break;
            case AP4_Track::TYPE_SYSTEM:    track_type = 'S'; break;
            case AP4_Track::TYPE_SUBTITLES: track_type = 'U'; break;
            default:                        track_type = ' '; break;
        }
        AP4_UI64 sample_dts_ms = AP4_ConvertTime(sample_dts, chosen_track->GetMediaTimeScale(), 1000);
        printf("%c %08d [%c] (%d)%c size=%6d, offset=%8lld, dts=%lld (%lld ms)\n",
               indicator?'|':' ', 
               i, 
               track_type, 
               chosen_track->GetId(),
               sample_is_sync?'*':' ', 
               sample_size, 
               sample_offset,
               sample_dts,
               sample_dts_ms);
    }
}

/*----------------------------------------------------------------------
|   ShowFragments_Text
+---------------------------------------------------------------------*/
static void
ShowFragments_Text(AP4_Movie& movie, bool verbose, bool show_sample_data, AP4_ByteStream* stream)
{
    stream->Seek(0);
    AP4_LinearReader reader(movie, stream);
    AP4_List<AP4_Track>::Item* track_item = movie.GetTracks().FirstItem();
    while (track_item) {
        reader.EnableTrack(track_item->GetData()->GetId());
        track_item = track_item->GetNext();
    }
    
    AP4_Sample     sample;
    AP4_DataBuffer sample_data;
    AP4_UI32       prev_track_id = 0;
    for(unsigned int i=0; ; i++) {
        AP4_UI32 track_id = 0;
        AP4_Result result = reader.ReadNextSample(sample, sample_data, track_id);
        if (AP4_SUCCEEDED(result)) {
            AP4_Track* track = movie.GetTrack(track_id);
            
            AP4_SampleDescription* sample_desc = track->GetSampleDescription(sample.GetDescriptionIndex());
            AP4_AvcSampleDescription* avc_desc = AP4_DYNAMIC_CAST(AP4_AvcSampleDescription, sample_desc);

            if (track_id != prev_track_id) {
                printf("Track %d:\n", track_id);
                prev_track_id = track_id;
            }
            
            ShowSample_Text(*track, sample, sample_data, i, verbose, show_sample_data, avc_desc);
            printf("\n");
        } else {
            break;
        }
    }
}

/*----------------------------------------------------------------------
|   main
+---------------------------------------------------------------------*/
int
main(int argc, char** argv)
{
    if (argc < 2) {
        PrintUsageAndExit();
    }
    Options.format = TEXT_FORMAT;
    const char* filename         = NULL;
    bool        verbose          = false;
    bool        show_samples     = false;
    bool        show_sample_data = false;
    bool        show_layout      = false;
    bool        fast             = false;
    
    while (char* arg = *++argv) {
        if (!strcmp(arg, "--verbose")) {
            verbose = true;
        } else if (!strcmp(arg, "--format")) {
            if (argv) {
                arg = *++argv;
                if (!strcmp(arg, "text")) {
                    Options.format = TEXT_FORMAT;
                } else if (!strcmp(arg, "json")) {
                    Options.format = JSON_FORMAT;
                } else {
                    fprintf(stderr, "ERROR: unsupported format\n");
                    return 1;
                }
            } else {
                fprintf(stderr, "ERROR: missing argument after '--format' option\n");
                return 1;
            }

        } else if (!strcmp(arg, "--fast")) {
            fast = true;
        } else if (!strcmp(arg, "--show-samples")) {
            show_samples = true;
        } else if (!strcmp(arg, "--show-sample-data")) {
            show_samples     = true;
            show_sample_data = true;
        } else if (!strcmp(arg, "--show-layout")) {
            show_layout = true;
        } else {
            if (filename == NULL) {
                filename = arg;
            } else {
                fprintf(stderr, "ERROR: unexpected argument '%s'\n", arg);
                return 1;
            }
        }   
    }
    if (filename == NULL) {
        fprintf(stderr, "ERROR: filename missing\n");
        return 1;
    }

    AP4_ByteStream* input = NULL;
    AP4_Result result = AP4_FileByteStream::Create(filename, 
                                                   AP4_FileByteStream::STREAM_MODE_READ, 
                                                   input);
    if (AP4_FAILED(result)) {
        fprintf(stderr, "ERROR: cannot open input file %s (%d)\n", filename, result);
        return 1;
    }

    if (Options.format == JSON_FORMAT) printf("{\n");
    
    AP4_File* file = new AP4_File(*input, true);
    ShowFileInfo(*file);

    AP4_Movie* movie = file->GetMovie();
    AP4_FtypAtom* ftyp = file->GetFileType();
    if (movie) {
        ShowMovieInfo(*movie);

        AP4_List<AP4_Track>& tracks = movie->GetTracks();
        if (Options.format == TEXT_FORMAT) {
            printf("Found %d Tracks\n", tracks.ItemCount());
        }

        if (ftyp && ftyp->GetMajorBrand() == AP4_MARLIN_BRAND_MGSV) {
            ShowMarlinTracks(*file, *input, tracks, show_samples, show_sample_data, verbose, fast);
        } else {
            ShowTracks(*movie, tracks, *input, show_samples, show_sample_data, verbose, fast);
        }
        
        if (show_layout) {
            ShowSampleLayout(tracks, verbose);
        }
        
        if (movie->HasFragments() && show_samples) {
            if (Options.format == TEXT_FORMAT) {
                ShowFragments_Text(*movie, verbose, show_sample_data, input);
            }
        }
    } else {
        // check if this is a DCF file
        if (ftyp && ftyp->GetMajorBrand() == AP4_OMA_DCF_BRAND_ODCF) {
            ShowDcfInfo(*file);
        } else {
            if (Options.format == TEXT_FORMAT) {
                printf("No movie found in the file\n");
            }
        }
    }
    
    if (Options.format == JSON_FORMAT) printf("}\n");

    input->Release();
    delete file;

    return 0;
}
