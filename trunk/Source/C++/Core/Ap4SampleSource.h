/*****************************************************************
|
|    AP4 - Sample Source Interface
|
|    Copyright 2002-2009 Axiomatic Systems, LLC
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

#ifndef _AP4_SAMPLE_SOURCE_H_
#define _AP4_SAMPLE_SOURCE_H_

/*----------------------------------------------------------------------
|   includes
+---------------------------------------------------------------------*/
#include "Ap4Types.h"

/*----------------------------------------------------------------------
|   class references
+---------------------------------------------------------------------*/
class AP4_Sample;
class AP4_SampleDescription;
class AP4_Track;
class AP4_DataBuffer;

/*----------------------------------------------------------------------
|   AP4_SampleSource
+---------------------------------------------------------------------*/
class AP4_SampleSource
{
public:
    virtual ~AP4_SampleSource() {}
    
    /**
     * Return the timscale of the sample's media
     */
    virtual AP4_UI32 GetTimeScale() = 0;
    
    /**
     * Return the duration in milliseconds
     */
    virtual AP4_UI32 GetDurationMs() = 0;
    
    /**
     * Read the next sample from the source
     */
    virtual AP4_Result ReadNextSample(AP4_Sample& sample, AP4_DataBuffer& buffer, AP4_UI32& track_id) = 0;
    
    /**
     * Seek to the sample closest to a specific timestamp in milliseconds
     */
    virtual AP4_Result SeekToTime(AP4_UI32 time_ms, bool before=true) = 0;
    
    /**
     * Return a sample description by index.
     * Returns NULL if there is no sample description with the requested index.
     */
    virtual AP4_SampleDescription* GetSampleDescription(AP4_Ordinal indx) = 0;
};

/*----------------------------------------------------------------------
|   AP4_TrackSampleSource
+---------------------------------------------------------------------*/
class AP4_TrackSampleSource : public AP4_SampleSource
{
public:
    AP4_TrackSampleSource(AP4_Track* track);     
    
    virtual AP4_UI32    GetTimeScale();
    virtual AP4_UI32    GetDurationMs();
    virtual AP4_Result  ReadNextSample(AP4_Sample& sample, AP4_DataBuffer& buffer, AP4_UI32& track_id);
    virtual AP4_Result  SeekToTime(AP4_UI32 time_ms, bool before=true) = 0;
    virtual AP4_SampleDescription* GetSampleDescription(AP4_Ordinal indx);
    
private:
    AP4_Track*  m_Track;
    AP4_Ordinal m_SampleIndex;
};

#endif // _AP4_SAMPLE_SOURCE_H_
