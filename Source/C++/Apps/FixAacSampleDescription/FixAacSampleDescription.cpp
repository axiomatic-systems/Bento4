/*****************************************************************
|
|    AP4 - Utility to Fix Sample Entries from MPEG-2 to MPEG-4
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
#include <stdio.h>
#include <stdlib.h>

#include "Ap4.h"

/*----------------------------------------------------------------------
|   constants
+---------------------------------------------------------------------*/
#define BANNER "MP4 Aac Sample Description Fixer - Version 1.0\n"\
    "(Bento4 Version " AP4_VERSION_STRING ")\n"\
    "(c) 2002-2008 Axiomatic Systems, LLC"

/*----------------------------------------------------------------------
|   PrintUsageAndExit
+---------------------------------------------------------------------*/
static void
PrintUsageAndExit()
{
    fprintf(stderr, 
        BANNER 
        "\n\nusage: fixaacsampledescription <input> <output>\n");
    exit(1);
}


/*----------------------------------------------------------------------
|   AP4_AacSampleDescritionFixer
+---------------------------------------------------------------------*/
class AP4_AacSampleDescriptionFixer : public AP4_Processor
{
public:
    virtual TrackHandler* CreateTrackHandler(AP4_TrakAtom* trak) {
        // find the stsd atom
        AP4_StsdAtom* stsd = AP4_DYNAMIC_CAST(AP4_StsdAtom, trak->FindChild("mdia/minf/stbl/stsd"));

        // avoid tracks with no stsd atom (should not happen)
        if (stsd == NULL) return NULL;

        // we only look at the first sample description
        AP4_SampleDescription* desc = stsd->GetSampleDescription(0);
        AP4_MpegAudioSampleDescription* audio_desc = 
            AP4_DYNAMIC_CAST(AP4_MpegAudioSampleDescription, desc);
        if (audio_desc && audio_desc->GetObjectTypeId()==AP4_OTI_MPEG2_AAC_AUDIO_LC) {
            // patch the stsd
            AP4_MpegAudioSampleDescription new_audio_desc(AP4_OTI_MPEG4_AUDIO,
                                                          audio_desc->GetSampleRate(), 
                                                          audio_desc->GetSampleSize(),
                                                          audio_desc->GetChannelCount(),
                                                          &audio_desc->GetDecoderInfo(),
                                                          audio_desc->GetBufferSize(),
                                                          audio_desc->GetMaxBitrate(),
                                                          audio_desc->GetAvgBitrate());
            stsd->RemoveChild(stsd->GetChild(AP4_ATOM_TYPE_MP4A));
            stsd->AddChild(new_audio_desc.ToAtom());
            printf("audio sample description patched\n");
        }
        return NULL;
    }
};


/*----------------------------------------------------------------------
|   main
+---------------------------------------------------------------------*/
int
main(int argc, char** argv)
{
    if (argc != 3) {
        PrintUsageAndExit();
    }

    // create the input stream
    AP4_Result result;
    AP4_ByteStream* input = NULL;
    result = AP4_FileByteStream::Create(argv[1], AP4_FileByteStream::STREAM_MODE_READ, input);
    if (AP4_FAILED(result)) {
        fprintf(stderr, "ERROR: cannot open input file (%s) %d\n", argv[1], result);
        return 1;
    }

    // create the output stream
    AP4_ByteStream* output = NULL;
    result = AP4_FileByteStream::Create(argv[2], AP4_FileByteStream::STREAM_MODE_WRITE, output);
    if (AP4_FAILED(result)) {
        fprintf(stderr, "ERROR: cannot open output file (%s) %d\n", argv[2], result);
        return 1;
    }

    // create the processor
    AP4_Processor* processor = new AP4_AacSampleDescriptionFixer;
    result = processor->Process(*input, *output, NULL);
    if (AP4_FAILED(result)) {
        fprintf(stderr, "ERROR: failed to process the file (%d)\n", result);
    }

    // cleanup
    delete processor;
    input->Release();
    output->Release();

    return 0;
}
