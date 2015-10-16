#include "Ap4.h"

// defined in jslib.js
extern "C" {
  extern void print_to_dom (const char* const);
}

void showAVCCodecString (const AP4_AvcSampleDescription& desc) {
  char buf [14];
  char coding [5];
  AP4_FormatFourChars(coding, desc.GetFormat());
  snprintf(buf, 14, "%s.%02X%02X%02X\n", coding, desc.GetProfile(),
      desc.GetProfileCompatibility(), desc.GetLevel());
  print_to_dom(buf);
}

void print_codec_string (AP4_Movie* const movie) {
  AP4_List<AP4_Track>& tracks = movie->GetTracks();
  //printf("found %d tracks\n", tracks.ItemCount());
  for (auto item = tracks.FirstItem(); item; item = item->GetNext()) {
    AP4_Track* track = item->GetData();
    AP4_SampleDescription* sample_desc = nullptr;
    for (unsigned int desc_index = 0;
        (sample_desc = track->GetSampleDescription(desc_index));
        ++desc_index) {
      if (sample_desc->GetType() == AP4_SampleDescription::TYPE_AVC) {
        AP4_AvcSampleDescription* avc_desc =
          AP4_DYNAMIC_CAST(AP4_AvcSampleDescription, sample_desc);
        print_to_dom("Codecs String:");
        showAVCCodecString(*avc_desc);
      }
      // we dont care about anything else, HEVC (h.265), audio, etc
    }
  }
}

extern "C" {

int dump_info (unsigned char* const input, unsigned int length) {
  AP4_DataBuffer buf { input, length };
  auto byte_stream = new AP4_MemoryByteStream(buf);
  AP4_File file { *byte_stream };
  byte_stream->Release();

  AP4_Movie* movie = file.GetMovie();
  if (movie) {
    print_codec_string(movie);
    char buf [23];
    snprintf(buf, 23, "fragmented: %s\n", &"false\0true"[6 *
        movie->HasFragments()]);
    print_to_dom(buf);
  }

  return 6745;
}

}

