/*****************************************************************
|
|    AP4 - File Processor
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
#include "Ap4Processor.h"
#include "Ap4AtomSampleTable.h"
#include "Ap4MovieFragment.h"
#include "Ap4FragmentSampleTable.h"
#include "Ap4TfhdAtom.h"
#include "Ap4AtomFactory.h"
#include "Ap4Movie.h"
#include "Ap4Array.h"
#include "Ap4Sample.h"
#include "Ap4TrakAtom.h"
#include "Ap4TfraAtom.h"
#include "Ap4TrunAtom.h"
#include "Ap4TrexAtom.h"
#include "Ap4TkhdAtom.h"
#include "Ap4SidxAtom.h"
#include "Ap4DataBuffer.h"
#include "Ap4Debug.h"

/*----------------------------------------------------------------------
|   types
+---------------------------------------------------------------------*/
struct AP4_SampleLocator {
    AP4_SampleLocator() : 
        m_TrakIndex(0), 
        m_SampleTable(NULL), 
        m_SampleIndex(0), 
        m_ChunkIndex(0) {}
    AP4_Ordinal          m_TrakIndex;
    AP4_AtomSampleTable* m_SampleTable;
    AP4_Ordinal          m_SampleIndex;
    AP4_Ordinal          m_ChunkIndex;
    AP4_Sample           m_Sample;
};

struct AP4_SampleCursor {
    AP4_SampleCursor() : m_EndReached(false) {}
    AP4_SampleLocator m_Locator;
    bool              m_EndReached;
};

struct AP4_AtomLocator {
    AP4_AtomLocator(AP4_Atom* atom, AP4_UI64 offset) : 
        m_Atom(atom),
        m_Offset(offset) {}
    AP4_Atom* m_Atom;
    AP4_UI64  m_Offset;
};

/*----------------------------------------------------------------------
|   AP4_DefaultFragmentHandler
+---------------------------------------------------------------------*/
class AP4_DefaultFragmentHandler: public AP4_Processor::FragmentHandler {
public:
    AP4_DefaultFragmentHandler(AP4_Processor::TrackHandler* track_handler) :
        m_TrackHandler(track_handler) {}
    AP4_Result ProcessSample(AP4_DataBuffer& data_in,
                             AP4_DataBuffer& data_out);
                             
private:
    AP4_Processor::TrackHandler* m_TrackHandler;
};

/*----------------------------------------------------------------------
|   AP4_DefaultFragmentHandler::ProcessSample
+---------------------------------------------------------------------*/
AP4_Result 
AP4_DefaultFragmentHandler::ProcessSample(AP4_DataBuffer& data_in, AP4_DataBuffer& data_out)
{
    if (m_TrackHandler == NULL) {
        data_out.SetData(data_in.GetData(), data_in.GetDataSize());
        return AP4_SUCCESS;
    }
    return m_TrackHandler->ProcessSample(data_in, data_out);
}

/*----------------------------------------------------------------------
|   FragmentMapEntry
+---------------------------------------------------------------------*/
typedef struct {
    AP4_UI64 before;
    AP4_UI64 after;
} FragmentMapEntry;

/*----------------------------------------------------------------------
|   FindFragmentMapEntry
+---------------------------------------------------------------------*/
static const FragmentMapEntry*
FindFragmentMapEntry(AP4_Array<FragmentMapEntry>& fragment_map, AP4_UI64 fragment_offset) {
    int first = 0;
    int last = fragment_map.ItemCount();
    while (first < last) {
        int middle = (last+first)/2;
        AP4_UI64 middle_value = fragment_map[middle].before;
        if (fragment_offset < middle_value) {
            last = middle;
        } else if (fragment_offset > middle_value) {
            first = middle+1;
        } else {
            return &fragment_map[middle];
        }
    }
    
    return NULL;
}

/*----------------------------------------------------------------------
|   AP4_Processor::ProcessFragments
+---------------------------------------------------------------------*/
AP4_Result
AP4_Processor::ProcessFragments(AP4_MoovAtom*              moov, 
                                AP4_List<AP4_AtomLocator>& atoms, 
                                AP4_ContainerAtom*         mfra,
                                AP4_SidxAtom*              sidx,
                                AP4_Position               sidx_position,
                                AP4_ByteStream&            input, 
                                AP4_ByteStream&            output)
{
    unsigned int fragment_index = 0;
    AP4_Array<FragmentMapEntry> fragment_map;
    
    for (AP4_List<AP4_AtomLocator>::Item* item = atoms.FirstItem();
                                          item;
                                          item = item->GetNext(), ++fragment_index) {
        AP4_AtomLocator*   locator     = item->GetData();
        AP4_Atom*          atom        = locator->m_Atom;
        AP4_UI64           atom_offset = locator->m_Offset;
        AP4_UI64           mdat_payload_offset = atom_offset+atom->GetSize()+AP4_ATOM_HEADER_SIZE;
        AP4_Sample         sample;
        AP4_DataBuffer     sample_data_in;
        AP4_DataBuffer     sample_data_out;
        AP4_Result         result;
    
        // if this is not a moof atom, just write it back and continue
        if (atom->GetType() != AP4_ATOM_TYPE_MOOF) {
            result = atom->Write(output);
            if (AP4_FAILED(result)) return result;
            continue;
        }
        
        // parse the moof
        AP4_ContainerAtom* moof = AP4_DYNAMIC_CAST(AP4_ContainerAtom, atom);
        AP4_MovieFragment* fragment = new AP4_MovieFragment(moof);

        // process all the traf atoms
        AP4_Array<AP4_Processor::FragmentHandler*> handlers;
        AP4_Array<AP4_FragmentSampleTable*> sample_tables;
        for (;AP4_Atom* child = moof->GetChild(AP4_ATOM_TYPE_TRAF, handlers.ItemCount());) {
            AP4_ContainerAtom* traf = AP4_DYNAMIC_CAST(AP4_ContainerAtom, child);
            AP4_TfhdAtom* tfhd = AP4_DYNAMIC_CAST(AP4_TfhdAtom, traf->GetChild(AP4_ATOM_TYPE_TFHD));
            
            // find the 'trak' for this track
            AP4_TrakAtom* trak = NULL;
            for (AP4_List<AP4_Atom>::Item* child_item = moov->GetChildren().FirstItem();
                                           child_item;
                                           child_item = child_item->GetNext()) {
                AP4_Atom* child_atom = child_item->GetData();
                if (child_atom->GetType() == AP4_ATOM_TYPE_TRAK) {
                    trak = AP4_DYNAMIC_CAST(AP4_TrakAtom, child_atom);
                    if (trak) {
                        AP4_TkhdAtom* tkhd = AP4_DYNAMIC_CAST(AP4_TkhdAtom, trak->GetChild(AP4_ATOM_TYPE_TKHD));
                        if (tkhd && tkhd->GetTrackId() == tfhd->GetTrackId()) {
                            break;
                        }
                    }
                    trak = NULL;
                }
            }
            
            // find the 'trex' for this track
            AP4_ContainerAtom* mvex = NULL;
            AP4_TrexAtom*      trex = NULL;
            mvex = AP4_DYNAMIC_CAST(AP4_ContainerAtom, moov->GetChild(AP4_ATOM_TYPE_MVEX));
            if (mvex) {
                for (AP4_List<AP4_Atom>::Item* child_item = mvex->GetChildren().FirstItem();
                                               child_item;
                                               child_item = child_item->GetNext()) {
                    AP4_Atom* child_atom = child_item->GetData();
                    if (child_atom->GetType() == AP4_ATOM_TYPE_TREX) {
                        trex = AP4_DYNAMIC_CAST(AP4_TrexAtom, child_atom);
                        if (trex && trex->GetTrackId() == tfhd->GetTrackId()) {
                            break;
                        }
                        trex = NULL;
                    }
                }
            }

            // create the handler for this traf
            AP4_Processor::FragmentHandler* handler = CreateFragmentHandler(trak, trex, traf, input, atom_offset);
            if (handler) {
                result = handler->ProcessFragment();
                if (AP4_FAILED(result)) return result;
            }
            handlers.Append(handler);
            
            // create a sample table object so we can read the sample data
            AP4_FragmentSampleTable* sample_table = NULL;
            result = fragment->CreateSampleTable(moov, tfhd->GetTrackId(), &input, atom_offset, mdat_payload_offset, 0, sample_table);
            if (AP4_FAILED(result)) return result;
            sample_tables.Append(sample_table);
            
            // let the handler look at the samples before we process them
            if (handler) result = handler->PrepareForSamples(sample_table);
            if (AP4_FAILED(result)) return result;
        }
             
        // write the moof
        AP4_UI64 moof_out_start = 0;
        output.Tell(moof_out_start);
        moof->Write(output);
        
        // remember the location of this fragment
        FragmentMapEntry map_entry = {atom_offset, moof_out_start};
        fragment_map.Append(map_entry);

        // write an mdat header
        AP4_Position mdat_out_start;
        AP4_UI64 mdat_size = AP4_ATOM_HEADER_SIZE;
        output.Tell(mdat_out_start);
        output.WriteUI32(0);
        output.WriteUI32(AP4_ATOM_TYPE_MDAT);

        // process all track runs
        for (unsigned int i=0; i<handlers.ItemCount(); i++) {
            AP4_Processor::FragmentHandler* handler = handlers[i];

            // get the track ID
            AP4_ContainerAtom* traf = AP4_DYNAMIC_CAST(AP4_ContainerAtom, moof->GetChild(AP4_ATOM_TYPE_TRAF, i));
            if (traf == NULL) continue;
            AP4_TfhdAtom* tfhd = AP4_DYNAMIC_CAST(AP4_TfhdAtom, traf->GetChild(AP4_ATOM_TYPE_TFHD));
            
            // compute the base data offset
            AP4_UI64 base_data_offset;
            if (tfhd->GetFlags() & AP4_TFHD_FLAG_BASE_DATA_OFFSET_PRESENT) {
                base_data_offset = mdat_out_start+AP4_ATOM_HEADER_SIZE;
            } else {
                base_data_offset = moof_out_start;
            }
            
            // build a list of all trun atoms
            AP4_Array<AP4_TrunAtom*> truns;
            for (AP4_List<AP4_Atom>::Item* child_item = traf->GetChildren().FirstItem();
                                           child_item;
                                           child_item = child_item->GetNext()) {
                AP4_Atom* child_atom = child_item->GetData();
                if (child_atom->GetType() == AP4_ATOM_TYPE_TRUN) {
                    AP4_TrunAtom* trun = AP4_DYNAMIC_CAST(AP4_TrunAtom, child_atom);
                    truns.Append(trun);
                }
            }    
            AP4_Ordinal   trun_index        = 0;
            AP4_Ordinal   trun_sample_index = 0;
            AP4_TrunAtom* trun = truns[0];
            trun->SetDataOffset((AP4_SI32)((mdat_out_start+mdat_size)-base_data_offset));
            
            // write the mdat
            for (unsigned int j=0; j<sample_tables[i]->GetSampleCount(); j++, trun_sample_index++) {
                // advance the trun index if necessary
                if (trun_sample_index >= trun->GetEntries().ItemCount()) {
                    trun = truns[++trun_index];
                    trun->SetDataOffset((AP4_SI32)((mdat_out_start+mdat_size)-base_data_offset));
                    trun_sample_index = 0;
                }
                
                // get the next sample
                result = sample_tables[i]->GetSample(j, sample);
                if (AP4_FAILED(result)) return result;
                sample.ReadData(sample_data_in);
                
                // process the sample data
                if (handler) {
                    result = handler->ProcessSample(sample_data_in, sample_data_out);
                    if (AP4_FAILED(result)) return result;

                    // write the sample data
                    result = output.Write(sample_data_out.GetData(), sample_data_out.GetDataSize());
                    if (AP4_FAILED(result)) return result;

                    // update the mdat size
                    mdat_size += sample_data_out.GetDataSize();
                    
                    // update the trun entry
                    trun->UseEntries()[trun_sample_index].sample_size = sample_data_out.GetDataSize();
                } else {
                    // write the sample data (unmodified)
                    result = output.Write(sample_data_in.GetData(), sample_data_in.GetDataSize());
                    if (AP4_FAILED(result)) return result;

                    // update the mdat size
                    mdat_size += sample_data_in.GetDataSize();
                }
            }

            if (handler) {
                // update the tfhd header
                if (tfhd->GetFlags() & AP4_TFHD_FLAG_BASE_DATA_OFFSET_PRESENT) {
                    tfhd->SetBaseDataOffset(mdat_out_start+AP4_ATOM_HEADER_SIZE);
                }
                if (tfhd->GetFlags() & AP4_TFHD_FLAG_DEFAULT_SAMPLE_SIZE_PRESENT) {
                    tfhd->SetDefaultSampleSize(trun->GetEntries()[0].sample_size);
                }
                
                // give the handler a chance to update the atoms
                handler->FinishFragment();
            }
        }

        // update the mdat header
        AP4_Position mdat_out_end;
        output.Tell(mdat_out_end);
#if defined(AP4_DEBUG)
        AP4_ASSERT(mdat_out_end-mdat_out_start == mdat_size);
#endif
        output.Seek(mdat_out_start);
        output.WriteUI32((AP4_UI32)mdat_size);
        output.Seek(mdat_out_end);
        
        // update the moof if needed
        output.Seek(moof_out_start);
        moof->Write(output);
        output.Seek(mdat_out_end);
        
        // update the sidx if we have one
        if (sidx && fragment_index < sidx->GetReferences().ItemCount()) {
            if (fragment_index == 0) {
                sidx->SetFirstOffset(moof_out_start-(sidx_position+sidx->GetSize()));
            }
            AP4_LargeSize fragment_size = mdat_out_end-moof_out_start;
            AP4_SidxAtom::Reference& sidx_ref = sidx->UseReferences()[fragment_index];
            sidx_ref.m_ReferencedSize = (AP4_UI32)fragment_size;
        }
        
        // cleanup
        delete fragment;
        
        for (unsigned int i=0; i<handlers.ItemCount(); i++) {
            delete handlers[i];
        }
        for (unsigned int i=0; i<sample_tables.ItemCount(); i++) {
            delete sample_tables[i];
        }
    }
    
    // update the mfra if we have one
    if (mfra) {
        for (AP4_List<AP4_Atom>::Item* mfra_item = mfra->GetChildren().FirstItem();
                                       mfra_item;
                                       mfra_item = mfra_item->GetNext()) {
            if (mfra_item->GetData()->GetType() != AP4_ATOM_TYPE_TFRA) continue;
            AP4_TfraAtom* tfra = AP4_DYNAMIC_CAST(AP4_TfraAtom, mfra_item->GetData());
            if (tfra == NULL) continue;
            AP4_Array<AP4_TfraAtom::Entry>& entries     = tfra->GetEntries();
            AP4_Cardinal                    entry_count = entries.ItemCount();
            for (unsigned int i=0; i<entry_count; i++) {
                const FragmentMapEntry* found = FindFragmentMapEntry(fragment_map, entries[i].m_MoofOffset);
                if (found) {
                    entries[i].m_MoofOffset = found->after;
                }
            }
        }
    }
    
    return AP4_SUCCESS;
}

/*----------------------------------------------------------------------
|   AP4_Processor::CreateFragmentHandler
+---------------------------------------------------------------------*/
AP4_Processor::FragmentHandler* 
AP4_Processor::CreateFragmentHandler(AP4_TrakAtom*      /* trak */,
                                     AP4_TrexAtom*      /* trex */,
                                     AP4_ContainerAtom* traf,
                                     AP4_ByteStream&    /* moof_data   */,
                                     AP4_Position       /* moof_offset */)
{
    // find the matching track handler
    for (unsigned int i=0; i<m_TrackIds.ItemCount(); i++) {
        AP4_TfhdAtom* tfhd = AP4_DYNAMIC_CAST(AP4_TfhdAtom, traf->GetChild(AP4_ATOM_TYPE_TFHD));
        if (tfhd && m_TrackIds[i] == tfhd->GetTrackId()) {
            return new AP4_DefaultFragmentHandler(m_TrackHandlers[i]);
        }
    }
    
    return NULL;
}

/*----------------------------------------------------------------------
|   AP4_Processor::Process
+---------------------------------------------------------------------*/
AP4_Result
AP4_Processor::Process(AP4_ByteStream&   input, 
                       AP4_ByteStream&   output,
                       AP4_ByteStream*   fragments,
                       ProgressListener* listener,
                       AP4_AtomFactory&  atom_factory)
{
    // read all atoms.
    // keep all atoms except [mdat]
    // keep a ref to [moov]
    // put [moof] atoms in a separate list
    AP4_AtomParent              top_level;
    AP4_MoovAtom*               moov = NULL;
    AP4_ContainerAtom*          mfra = NULL;
    AP4_SidxAtom*               sidx = NULL;
    AP4_List<AP4_AtomLocator>   frags;
    AP4_UI64                    stream_offset = 0;
    bool                        in_fragments = false;
    unsigned int                sidx_count = 0;
    for (AP4_Atom* atom = NULL;
        AP4_SUCCEEDED(atom_factory.CreateAtomFromStream(input, atom));
        input.Tell(stream_offset)) {
        if (atom->GetType() == AP4_ATOM_TYPE_MDAT) {
            delete atom;
            continue;
        } else if (atom->GetType() == AP4_ATOM_TYPE_MOOV) {
            moov = AP4_DYNAMIC_CAST(AP4_MoovAtom, atom);
            if (fragments) break;
        } else if (atom->GetType() == AP4_ATOM_TYPE_MFRA) {
            mfra = AP4_DYNAMIC_CAST(AP4_ContainerAtom, atom);
            continue;
        } else if (atom->GetType() == AP4_ATOM_TYPE_SIDX) {
            // don't keep the index, it is likely to be invalidated, we will recompute it later
            ++sidx_count;
            if (sidx == NULL) {
                sidx = AP4_DYNAMIC_CAST(AP4_SidxAtom, atom);
            } else {
                delete atom;
                continue;
            }
        } else if (atom->GetType() == AP4_ATOM_TYPE_SSIX) {
            // don't keep the index, it is likely to be invalidated
            delete atom;
            continue;
        } else if (!fragments && (in_fragments || atom->GetType() == AP4_ATOM_TYPE_MOOF)) {
            in_fragments = true;
            frags.Add(new AP4_AtomLocator(atom, stream_offset));
            continue;
        }
        top_level.AddChild(atom);
    }

    // check that we have at most one sidx (we can't deal with multi-sidx streams here
    if (sidx_count > 1) {
        top_level.RemoveChild(sidx);
        delete sidx;
        sidx = NULL;
    }
    
    // if we have a fragments stream, get the fragment locators from there
    if (fragments) {
        stream_offset = 0;
        for (AP4_Atom* atom = NULL;
            AP4_SUCCEEDED(atom_factory.CreateAtomFromStream(*fragments, atom));
            fragments->Tell(stream_offset)) {
            if (atom->GetType() == AP4_ATOM_TYPE_MDAT) {
                delete atom;
                continue;
            }
            frags.Add(new AP4_AtomLocator(atom, stream_offset));
        }
    }
    
    // initialize the processor
    AP4_Result result = Initialize(top_level, input);
    if (AP4_FAILED(result)) return result;

    // process the tracks if we have a moov atom
    AP4_Array<AP4_SampleLocator> locators;
    AP4_Cardinal                 track_count       = 0;
    AP4_List<AP4_TrakAtom>*      trak_atoms        = NULL;
    AP4_LargeSize                mdat_payload_size = 0;
    AP4_SampleCursor*            cursors           = NULL;
    if (moov) {
        // build an array of track sample locators
        trak_atoms = &moov->GetTrakAtoms();
        track_count = trak_atoms->ItemCount();
        cursors = new AP4_SampleCursor[track_count];
        m_TrackHandlers.SetItemCount(track_count);
        m_TrackIds.SetItemCount(track_count);
        for (AP4_Ordinal i=0; i<track_count; i++) {
            m_TrackHandlers[i] = NULL;
            m_TrackIds[i] = 0;
        }
        
        unsigned int index = 0;
        for (AP4_List<AP4_TrakAtom>::Item* item = trak_atoms->FirstItem(); item; item=item->GetNext()) {
            AP4_TrakAtom* trak = item->GetData();

            // find the stsd atom
            AP4_ContainerAtom* stbl = AP4_DYNAMIC_CAST(AP4_ContainerAtom, trak->FindChild("mdia/minf/stbl"));
            if (stbl == NULL) continue;
            
            // see if there's an external data source for this track
            AP4_ByteStream* trak_data_stream = &input;
            for (AP4_List<ExternalTrackData>::Item* ditem = m_ExternalTrackData.FirstItem(); ditem; ditem=ditem->GetNext()) {
                ExternalTrackData* tdata = ditem->GetData();
                if (tdata->m_TrackId == trak->GetId()) {
                    trak_data_stream = tdata->m_MediaData;
                    break;
                }
            }

            // create the track handler    
            m_TrackHandlers[index] = CreateTrackHandler(trak);
            m_TrackIds[index]      = trak->GetId();
            cursors[index].m_Locator.m_TrakIndex   = index;
            cursors[index].m_Locator.m_SampleTable = new AP4_AtomSampleTable(stbl, *trak_data_stream);
            cursors[index].m_Locator.m_SampleIndex = 0;
            cursors[index].m_Locator.m_ChunkIndex  = 0;
            if (cursors[index].m_Locator.m_SampleTable->GetSampleCount()) {
                cursors[index].m_Locator.m_SampleTable->GetSample(0, cursors[index].m_Locator.m_Sample);
            } else {
                cursors[index].m_EndReached = true;
            }

            index++;            
        }

        // figure out the layout of the chunks
        for (;;) {
            // see which is the next sample to write
            AP4_UI64 min_offset = (AP4_UI64)(-1);
            int cursor = -1;
            for (unsigned int i=0; i<track_count; i++) {
                if (!cursors[i].m_EndReached &&
                    cursors[i].m_Locator.m_SampleTable &&
                    cursors[i].m_Locator.m_Sample.GetOffset() <= min_offset) {
                    min_offset = cursors[i].m_Locator.m_Sample.GetOffset();
                    cursor = i;
                }
            }

            // stop if all cursors are exhausted
            if (cursor == -1) break;

            // append this locator to the layout list
            AP4_SampleLocator& locator = cursors[cursor].m_Locator;
            locators.Append(locator);

            // move the cursor to the next sample
            locator.m_SampleIndex++;
            if (locator.m_SampleIndex == locator.m_SampleTable->GetSampleCount()) {
                // mark this track as completed
                cursors[cursor].m_EndReached = true;
            } else {
                // get the next sample info
                locator.m_SampleTable->GetSample(locator.m_SampleIndex, locator.m_Sample);
                AP4_Ordinal skip, sdesc;
                locator.m_SampleTable->GetChunkForSample(locator.m_SampleIndex,
                                                         locator.m_ChunkIndex,
                                                         skip, sdesc);
            }
        }

        // update the stbl atoms and compute the mdat size
        int current_track = -1;
        int current_chunk = -1;
        AP4_Position current_chunk_offset = 0;
        AP4_Size current_chunk_size = 0;
        for (AP4_Ordinal i=0; i<locators.ItemCount(); i++) {
            AP4_SampleLocator& locator = locators[i];
            if ((int)locator.m_TrakIndex  != current_track ||
                (int)locator.m_ChunkIndex != current_chunk) {
                // start a new chunk for this track
                current_chunk_offset += current_chunk_size;
                current_chunk_size = 0;
                current_track = locator.m_TrakIndex;
                current_chunk = locator.m_ChunkIndex;
                locator.m_SampleTable->SetChunkOffset(locator.m_ChunkIndex, current_chunk_offset);
            } 
            AP4_Size sample_size;
            TrackHandler* handler = m_TrackHandlers[locator.m_TrakIndex];
            if (handler) {
                sample_size = handler->GetProcessedSampleSize(locator.m_Sample);
                locator.m_SampleTable->SetSampleSize(locator.m_SampleIndex, sample_size);
            } else {
                sample_size = locator.m_Sample.GetSize();
            }
            current_chunk_size += sample_size;
            mdat_payload_size  += sample_size;
        }

        // process the tracks (ex: sample descriptions processing)
        for (AP4_Ordinal i=0; i<track_count; i++) {
            TrackHandler* handler = m_TrackHandlers[i];
            if (handler) handler->ProcessTrack();
        }
    }

    // finalize the processor
    Finalize(top_level);

    if (!fragments) {
        // calculate the size of all atoms combined
        AP4_UI64 atoms_size = 0;
        top_level.GetChildren().Apply(AP4_AtomSizeAdder(atoms_size));

        // see if we need a 64-bit or 32-bit mdat
        AP4_Size mdat_header_size = AP4_ATOM_HEADER_SIZE;
        if (mdat_payload_size+mdat_header_size > 0xFFFFFFFF) {
            // we need a 64-bit size
            mdat_header_size += 8;
        }
        
        // adjust the chunk offsets
        for (AP4_Ordinal i=0; i<track_count; i++) {
            AP4_TrakAtom* trak;
            trak_atoms->Get(i, trak);
            trak->AdjustChunkOffsets(atoms_size+mdat_header_size);
        }

        // write all atoms
        top_level.GetChildren().Apply(AP4_AtomListWriter(output));

        // write mdat header
        if (mdat_payload_size) {
            if (mdat_header_size == AP4_ATOM_HEADER_SIZE) {
                // 32-bit size
                output.WriteUI32((AP4_UI32)(mdat_header_size+mdat_payload_size));
                output.WriteUI32(AP4_ATOM_TYPE_MDAT);
            } else {
                // 64-bit size
                output.WriteUI32(1);
                output.WriteUI32(AP4_ATOM_TYPE_MDAT);
                output.WriteUI64(mdat_header_size+mdat_payload_size);
            }
        }        
    }
    
    // write the samples
    if (moov) {
        if (!fragments) {
#if defined(AP4_DEBUG)
            AP4_Position before;
            output.Tell(before);
#endif
            AP4_Sample     sample;
            AP4_DataBuffer data_in;
            AP4_DataBuffer data_out;
            for (unsigned int i=0; i<locators.ItemCount(); i++) {
                AP4_SampleLocator& locator = locators[i];
                locator.m_Sample.ReadData(data_in);
                TrackHandler* handler = m_TrackHandlers[locator.m_TrakIndex];
                if (handler) {
                    result = handler->ProcessSample(data_in, data_out);
                    if (AP4_FAILED(result)) return result;
                    output.Write(data_out.GetData(), data_out.GetDataSize());
                } else {
                    output.Write(data_in.GetData(), data_in.GetDataSize());            
                }

                // notify the progress listener
                if (listener) {
                    listener->OnProgress(i+1, locators.ItemCount());
                }
            }

#if defined(AP4_DEBUG)
            AP4_Position after;
            output.Tell(after);
            AP4_ASSERT(after-before == mdat_payload_size);
#endif
        }
        
        // find the position of the sidx atom
        AP4_Position sidx_position = 0;
        if (sidx) {
            for (AP4_List<AP4_Atom>::Item* item = top_level.GetChildren().FirstItem();
                                           item;
                                           item = item->GetNext()) {
                AP4_Atom* atom = item->GetData();
                if (atom->GetType() == AP4_ATOM_TYPE_SIDX) {
                    break;
                }
                sidx_position += atom->GetSize();
            }
        }
        
        // process the fragments, if any
        result = ProcessFragments(moov, frags, mfra, sidx, sidx_position, fragments?*fragments:input, output);
        if (AP4_FAILED(result)) return result;
        
        // update and re-write the sidx if we have one
        if (sidx && sidx_position) {
            AP4_Position where = 0;
            output.Tell(where);
            output.Seek(sidx_position);
            result = sidx->Write(output);
            if (AP4_FAILED(result)) return result;
            output.Seek(where);
        }
        
        if (!fragments) {
            // write the mfra atom at the end if we have one
            if (mfra) {
                mfra->Write(output);
            }
        }
        
        // cleanup
        for (AP4_Ordinal i=0; i<track_count; i++) {
            delete cursors[i].m_Locator.m_SampleTable;
            delete m_TrackHandlers[i];
        }
        m_TrackHandlers.Clear();
        delete[] cursors;
    }

    // cleanup
    frags.DeleteReferences();
    delete mfra;
    if (fragments) {
        // with a fragments stream, `moov` isn't inclued in `top_level`
        // so we need to delete it here
        delete moov;
    }
    
    return AP4_SUCCESS;
}

/*----------------------------------------------------------------------
|   AP4_Processor::Process
+---------------------------------------------------------------------*/
AP4_Result
AP4_Processor::Process(AP4_ByteStream&   input, 
                       AP4_ByteStream&   output,
                       ProgressListener* listener,
                       AP4_AtomFactory&  atom_factory)
{
    return Process(input, output, NULL, listener, atom_factory);
}

/*----------------------------------------------------------------------
|   AP4_Processor::Process
+---------------------------------------------------------------------*/
AP4_Result
AP4_Processor::Process(AP4_ByteStream&   fragments, 
                       AP4_ByteStream&   output,
                       AP4_ByteStream&   init,
                       ProgressListener* listener,
                       AP4_AtomFactory&  atom_factory)
{
    return Process(init, output, &fragments, listener, atom_factory);
}

/*----------------------------------------------------------------------
|   AP4_Processor:Initialize
+---------------------------------------------------------------------*/
AP4_Result 
AP4_Processor::Initialize(AP4_AtomParent&   /* top_level */,
                          AP4_ByteStream&   /* stream    */,
                          ProgressListener* /* listener  */)
{
    // default implementation: do nothing
    return AP4_SUCCESS;
}

/*----------------------------------------------------------------------
|   AP4_Processor:Finalize
+---------------------------------------------------------------------*/
AP4_Result 
AP4_Processor::Finalize(AP4_AtomParent&   /* top_level */,
                        ProgressListener* /* listener */ )
{
    // default implementation: do nothing
    return AP4_SUCCESS;
}

/*----------------------------------------------------------------------
|   AP4_Processor::TrackHandler Dynamic Cast Anchor
+---------------------------------------------------------------------*/
AP4_DEFINE_DYNAMIC_CAST_ANCHOR_S(AP4_Processor::TrackHandler, TrackHandler)
