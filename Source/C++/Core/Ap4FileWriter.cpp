/*****************************************************************
|
|    AP4 - File Writer
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
#include "Ap4MoovAtom.h"
#include "Ap4FileWriter.h"
#include "Ap4Movie.h"
#include "Ap4File.h"
#include "Ap4TrakAtom.h"
#include "Ap4Track.h"
#include "Ap4Sample.h"
#include "Ap4DataBuffer.h"
#include "Ap4FtypAtom.h"
#include "Ap4SampleTable.h"
#include "Ap4StcoAtom.h"
#include "Ap4Co64Atom.h"

/*----------------------------------------------------------------------
|   AP4_FileWriter::Write
+---------------------------------------------------------------------*/
AP4_Result
AP4_FileWriter::Write(AP4_File& file, AP4_ByteStream& stream, Interleaving /* interleaving */)
{
    // get the file type
    AP4_FtypAtom* file_type = file.GetFileType();

    // write the ftyp atom (always first)
    if (file_type) file_type->Write(stream);

    // write the top-level atoms, except for ftyp, moov and mdat
    for (AP4_List<AP4_Atom>::Item* atom_item = file.GetChildren().FirstItem();
         atom_item;
         atom_item = atom_item->GetNext()) {
        AP4_Atom* atom = atom_item->GetData();
        if (atom->GetType() != AP4_ATOM_TYPE_MDAT &&
            atom->GetType() != AP4_ATOM_TYPE_FTYP &&
            atom->GetType() != AP4_ATOM_TYPE_MOOV) {
            atom->Write(stream);
        }
    }
             
    // get the movie object (and stop if there isn't any)
    AP4_Movie* movie = file.GetMovie();
    if (movie == NULL) return AP4_SUCCESS;

    // see how much we've written so far
    AP4_Position position;
    stream.Tell(position);
    
    // backup and recompute all the chunk offsets
    unsigned int t=0;
    AP4_Result result = AP4_SUCCESS;
    AP4_UI64   mdat_size = AP4_ATOM_HEADER_SIZE;
    // compute mdat_size
    for (AP4_List<AP4_Track>::Item* track_item = movie->GetTracks().FirstItem();
         track_item; track_item = track_item->GetNext()) {
        AP4_Track*    track = track_item->GetData();
        AP4_Cardinal     sample_count = track->GetSampleCount();
        AP4_SampleTable* sample_table = track->GetSampleTable();
        AP4_Sample       sample;
        for (AP4_Ordinal i=0; i<sample_count; i++) {
            sample_table->GetSample(i, sample);
            mdat_size += sample.GetSize();
        }
    }

    // replace stco with co64
    if (mdat_size >> 32) {
        for (AP4_List<AP4_Track>::Item* track_item = movie->GetTracks().FirstItem();
             track_item; track_item = track_item->GetNext()) {
            AP4_Track*    track = track_item->GetData();
            AP4_TrakAtom* trak  = const_cast<AP4_TrakAtom*>(track->GetTrakAtom());
            AP4_StcoAtom* stco = AP4_DYNAMIC_CAST(AP4_StcoAtom, trak->FindChild("mdia/minf/stbl/stco"));
            if (stco) {
                AP4_Cardinal chunk_count = stco->GetChunkCount();
                AP4_UI64* chunk_offsets = new AP4_UI64[chunk_count];
                AP4_UI32* chunk_offsets_32 = stco->GetChunkOffsets();
                for (unsigned int i=0; i<chunk_count; i++) {
                    chunk_offsets[i] =  chunk_offsets_32[i];
                }

                AP4_Co64Atom* co64 = new AP4_Co64Atom(chunk_offsets, chunk_count);
                delete[] chunk_offsets;
                stco->Detach();
                delete stco;

                AP4_ContainerAtom* stbl = AP4_DYNAMIC_CAST(AP4_ContainerAtom, trak->FindChild("mdia/minf/stbl"));
                if (stbl == NULL) {
                	delete co64;
                	return AP4_ERROR_INTERNAL;
                }
               	stbl->AddChild(co64, -1);
            }
        }
        mdat_size = AP4_ATOM_HEADER_SIZE_64;
    } else {
        mdat_size = AP4_ATOM_HEADER_SIZE;
    }

    AP4_UI64   mdat_position = position+movie->GetMoovAtom()->GetSize();
    AP4_Array<AP4_Array<AP4_UI64>*> trak_chunk_offsets_backup;
    AP4_Array<AP4_UI64>             chunk_offsets;
    for (AP4_List<AP4_Track>::Item* track_item = movie->GetTracks().FirstItem();
                                    track_item;
                                    track_item = track_item->GetNext()) {
        AP4_Track*    track = track_item->GetData();
        AP4_TrakAtom* trak  = track->UseTrakAtom();
        
        // backup the chunk offsets
        AP4_Array<AP4_UI64>* chunk_offsets_backup = new AP4_Array<AP4_UI64>();
        trak_chunk_offsets_backup.Append(chunk_offsets_backup);
        result = trak->GetChunkOffsets(*chunk_offsets_backup);
        if (AP4_FAILED(result)) goto end;

        // allocate space for the new chunk offsets
        chunk_offsets.SetItemCount(chunk_offsets_backup->ItemCount());
        
        // compute the new chunk offsets
        AP4_Cardinal     sample_count = track->GetSampleCount();
        AP4_SampleTable* sample_table = track->GetSampleTable();
        AP4_Sample       sample;
        for (AP4_Ordinal i=0; i<sample_count; i++) {
            AP4_Ordinal chunk_index = 0;
            AP4_Ordinal position_in_chunk = 0;
            sample_table->GetSampleChunkPosition(i, chunk_index, position_in_chunk);
            sample_table->GetSample(i, sample);
            if (position_in_chunk == 0) {
                // this sample is the first sample in a chunk, so this is the start of a chunk
                if (chunk_index >= chunk_offsets.ItemCount()) return AP4_ERROR_INTERNAL;
                chunk_offsets[chunk_index] = mdat_position+mdat_size;
            }
            mdat_size += sample.GetSize();
        }        
        result = trak->SetChunkOffsets(chunk_offsets);
    }
    
    // write the moov atom
    movie->GetMoovAtom()->Write(stream);
    
    // create and write the media data (mdat)
    if ((mdat_size >> 32) == 0) {
    	stream.WriteUI32((AP4_UI32)mdat_size);
    	stream.WriteUI32(AP4_ATOM_TYPE_MDAT);
    } else {
    	stream.WriteUI32(1);
    	stream.WriteUI32(AP4_ATOM_TYPE_MDAT);
    	stream.WriteUI64(mdat_size);
    }
    
    // write all tracks and restore the chunk offsets to their backed-up values
    for (AP4_List<AP4_Track>::Item* track_item = movie->GetTracks().FirstItem();
         track_item;
         track_item = track_item->GetNext(), ++t) {
        AP4_Track*    track = track_item->GetData();
        AP4_TrakAtom* trak  = track->UseTrakAtom();
        
        // restore the backed-up chunk offsets
        result = trak->SetChunkOffsets(*trak_chunk_offsets_backup[t]);

        // write all the track's samples
        AP4_Cardinal   sample_count = track->GetSampleCount();
        AP4_Sample     sample;
        AP4_DataBuffer sample_data;
        for (AP4_Ordinal i=0; i<sample_count; i++) {
            track->ReadSample(i, sample, sample_data);
            stream.Write(sample_data.GetData(), sample_data.GetDataSize());
        }
    }

end:
    for (unsigned int i=0; i<trak_chunk_offsets_backup.ItemCount(); i++) {
        delete trak_chunk_offsets_backup[i];
    }
    
    return result;
}
