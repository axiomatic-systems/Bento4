/*****************************************************************
|
|    AP4 - Benchmarks Test
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
#if !defined _REENTRANT
#define _REENTRANT
#endif
#if defined (WIN32)
#include <sys/timeb.h>
#else
#include <sys/time.h>
#endif

#include "Ap4.h"
#include "Ap4StreamCipher.h"

/*----------------------------------------------------------------------
|   constants
+---------------------------------------------------------------------*/
#define TIME_SPAN 30.0  /* seconds */
#define ENC_IN_BUFFER_SIZE (1024*128)
#define ENC_OUT_BUFFER_SIZE (ENC_IN_BUFFER_SIZE+32)
#define SCALE_MB (1024.0f*1024.0f)

/*----------------------------------------------------------------------
|   macros
+---------------------------------------------------------------------*/
#define BENCH_START(_msg, _select)      \
if (_select) {                          \
    double before = GetTime();          \
    double after = 0.0;                 \
    double total = 0.0;                 \
    printf("%s:", _msg);                \
    fflush(stdout);                     \
    unsigned int i;                     \
    for (i=0; (i<max_iterations) && ((after=GetTime())-before)<max_time; i++) { 

#define BENCH_END(_unit, _scale)                          \
    }                                                     \
    double time_diff = after-before;                      \
    double st = total/(_scale);                           \
    double stps = (st/time_diff);                         \
    printf(" %f " _unit "/s (%f " _unit " in %f seconds, %d iterations)\n", stps, st, time_diff, i); \
}

#if defined(WIN32)
/*----------------------------------------------------------------------
|   GetTime
+---------------------------------------------------------------------*/
static double
GetTime()
{
    struct _timeb time_stamp;

#if defined(_MSC_VER) && (_MSC_VER >= 1400)
    _ftime_s(&time_stamp);
#else
    _ftime(&time_stamp);
#endif
    double nowf = (double)time_stamp.time+((double)time_stamp.millitm)/1000.0f;
    return nowf;
}
#else
/*----------------------------------------------------------------------
|   GetTime
+---------------------------------------------------------------------*/
static double
GetTime()
{
    struct timeval now;
    gettimeofday(&now, NULL);
    double nowf = (double)now.tv_sec+((double)now.tv_usec)/1000000.0f;
    //printf("%f\n", nowf);
    return nowf;
}
#endif

/*----------------------------------------------------------------------
|   PrintUsage
+---------------------------------------------------------------------*/
static void
PrintUsage()
{
    printf("benchmarktest [options] <test-name> [, <test-name>, ...]\n"
           "options:\n"
           "  --iterations=<n>: run each test for <n> iterations instead of a fixed run time.\n"
           "  --test-file-read=<filename> (any file for read tests)\n"
           "  --test-file-mp4=<filename> (MP4 file for parse-file, parse-samples and read-samples)\n"
           "  --test-file-dcf-cbc=<filename> (DCF/CBC file for read-samples-dcf-cbc)\n"
           "  --test-file-dcf-ctr=<filename> (DCF/CTR file for read-samples-dcf-ctr)\n"
           "  --test-file-pdcf-cbc=<filename> (PDCF/CBC file for read-samples-pdcf-cbc)\n"
           "  --test-file-pdcf-ctr=<filename> (PDCF/CTR file for read-samples-pdcf-ctr)\n"
           "\n"
           "valid test names are:\n"
           "all: run all tests\n"
           "or one or more of the following tests:\n"
           "aes-cbc-block-decrypt\n"
           "aes-cbc-block-encrypt\n"
           "aes-ctr-block\n"
           "aes-cbc-stream-encrypt\n"
           "aes-cbc-stream-decrypt\n"
           "aes-ctr-stream\n"
           "parse-file\n"
           "parse-file-buffered\n"
           "parse-samples\n"
           "read-file-seq-1\n"
           "read-file-seq-16\n"
           "read-file-seq-256\n"
           "read-file-seq-4096\n"
           "read-file-rnd-1\n"
           "read-file-rnd-16\n"
           "read-file-rnd-256\n"
           "read-file-rnd-4096\n"
           "read-samples\n"
           "read-samples-dcf-cbc\n"
           "read-samples-dcf-ctr\n"
           "read-samples-pdcf-cbc\n"
           "read-samples-pdcf-ctr\n");
}

/*----------------------------------------------------------------------
|   CreateDcfDecrypter
+---------------------------------------------------------------------*/
static AP4_ByteStream*
CreateDcfDecrypter(AP4_AtomParent& atoms, const AP4_Byte* key)
{
    for (AP4_List<AP4_Atom>::Item* item = atoms.GetChildren().FirstItem();
         item;
        item = item->GetNext()) {
        AP4_Atom* atom = item->GetData();
        if (atom->GetType() != AP4_ATOM_TYPE_ODRM) continue;

        // check that we have all the atoms we need
        AP4_ContainerAtom* odrm = AP4_DYNAMIC_CAST(AP4_ContainerAtom, atom);
        if (odrm == NULL) continue; // not enough info
        AP4_OdheAtom* odhe = AP4_DYNAMIC_CAST(AP4_OdheAtom, odrm->GetChild(AP4_ATOM_TYPE_ODHE));
        if (odhe == NULL) continue; // not enough info    
        AP4_OddaAtom* odda = AP4_DYNAMIC_CAST(AP4_OddaAtom, odrm->GetChild(AP4_ATOM_TYPE_ODDA));;
        if (odda == NULL) continue; // not enough info
        AP4_OhdrAtom* ohdr = AP4_DYNAMIC_CAST(AP4_OhdrAtom, odhe->GetChild(AP4_ATOM_TYPE_OHDR));
        if (ohdr == NULL) continue; // not enough info

        // do nothing if the atom is not encrypted
        if (ohdr->GetEncryptionMethod() == AP4_OMA_DCF_ENCRYPTION_METHOD_NULL) {
            continue;
        }
                
        // create the byte stream
        AP4_ByteStream* cipher_stream = NULL;
        AP4_Result result = AP4_OmaDcfAtomDecrypter::CreateDecryptingStream(*odrm, 
                                                   key, 
                                                   16, 
                                                   NULL, 
                                                   cipher_stream);
        if (AP4_SUCCEEDED(result)) {
            return cipher_stream;
        }
    }

    return NULL;
}

/*----------------------------------------------------------------------
|   LoadAndDecryptSamples
+---------------------------------------------------------------------*/
static unsigned int
LoadAndDecryptSamples(AP4_Track*             track, 
                      AP4_SampleDescription* sample_desc,
                      unsigned int           repeats)
{
    AP4_ProtectedSampleDescription* pdesc = 
        AP4_DYNAMIC_CAST(AP4_ProtectedSampleDescription, sample_desc);

    const AP4_UI08 key[16] = {0x00, 0x01, 0x02, 0x03, 0x04, 0x05, 0x06, 0x07, 0x08, 0x09, 0x0a, 0x0b, 0x0c, 0x0d, 0x0e, 0x0f};
    AP4_SampleDecrypter* decrypter = AP4_SampleDecrypter::Create(pdesc, key, 16);

    unsigned int total_size = 0;
    while (repeats--) {
        AP4_Sample     sample;
        AP4_DataBuffer encrypted_data;
        AP4_DataBuffer decrypted_data;
        AP4_Ordinal    index = 0;
        while (AP4_SUCCEEDED(track->ReadSample(index, sample, encrypted_data))) {
            if (AP4_FAILED(decrypter->DecryptSampleData(encrypted_data, decrypted_data))) {
                fprintf(stderr, "ERROR: failed to decrypt sample\n");
                return 0;
            }
            total_size += decrypted_data.GetDataSize();
            index++;
        }
    }
    return total_size;
}

/*----------------------------------------------------------------------
|   LoadSamples
+---------------------------------------------------------------------*/
static unsigned int
LoadSamples(AP4_Track* track, unsigned int repeats)
{
    AP4_SampleDescription* sample_desc = track->GetSampleDescription(0);
    if (sample_desc->GetType() == AP4_SampleDescription::TYPE_PROTECTED) {
        return LoadAndDecryptSamples(track, sample_desc, repeats);
    } else if (track->GetType() != AP4_Track::TYPE_AUDIO &&
               track->GetType() != AP4_Track::TYPE_VIDEO) {
        return 0;
    }
    
    unsigned int total_size = 0;
    while (repeats--) {
        AP4_Sample     sample;
        AP4_DataBuffer sample_data;
        AP4_Ordinal    index = 0;
        while (AP4_SUCCEEDED(track->ReadSample(index, sample, sample_data))) {
            total_size += sample_data.GetDataSize();
            index++;
        }
    }
    
    return total_size;
}

/*----------------------------------------------------------------------
|   ParseSamples
+---------------------------------------------------------------------*/
static unsigned int
ParseSamples(AP4_Track* track, unsigned int repeats)
{
    unsigned int total_count = 0;
    while (repeats--) {
        AP4_Sample  sample;
        AP4_Ordinal index = 0;
        while (AP4_SUCCEEDED(track->GetSample(index, sample))) {
            ++total_count;
            ++index;
        }
    }
    
    return total_count;
}

/*----------------------------------------------------------------------
|   ParseAllSamples
+---------------------------------------------------------------------*/
static unsigned int
ParseAllSamples(const char* filename, unsigned int repeats)
{
    // open the input
    AP4_ByteStream* input = NULL;
    AP4_Result result = AP4_FileByteStream::Create(filename, AP4_FileByteStream::STREAM_MODE_READ, input);
    if (AP4_FAILED(result)) {
        fprintf(stderr, "ERROR: cannot open input file (%s)\n", filename);
        return 0;
    }
        
    // parse the file
    AP4_File* mp4_file = new AP4_File(*input);
    
    // parse all the tracks that need to be parsed
    unsigned int total_count = 0;
    for (AP4_List<AP4_Track>::Item* item = mp4_file->GetMovie()->GetTracks().FirstItem(); item; item=item->GetNext()) {
        AP4_Track* track = item->GetData();
        total_count += ParseSamples(track, repeats);
    }

    delete mp4_file;
    input->Release();
    
    return total_count;
}

/*----------------------------------------------------------------------
|   ParseFile
+---------------------------------------------------------------------*/
static unsigned int
ParseFile(const char* filename, unsigned int repeats, bool buffered)
{
    unsigned int total_size = 0;
    
    // open the input
    AP4_ByteStream* input = NULL;
    AP4_Result result = AP4_FileByteStream::Create(filename, AP4_FileByteStream::STREAM_MODE_READ, input);
    if (AP4_FAILED(result)) {
        fprintf(stderr, "ERROR: cannot open input file (%s)\n", filename);
        return 0;
    }
    if (buffered) {
        AP4_BufferedInputStream* buffered = new AP4_BufferedInputStream(*input);
        input->Release();
        input = buffered;
    }
        
    for (unsigned int i=0; i<repeats; i++) {
        // parse the file
        AP4_File* mp4_file = new AP4_File(*input, true);
        AP4_Movie* movie = mp4_file->GetMovie();
        if (movie) {
            total_size += movie->GetMoovAtom()->GetSize();
        }
        delete mp4_file;
        input->Seek(0);
    }

    input->Release();
    
    return total_size;
}

/*----------------------------------------------------------------------
|   LoadAllSamples
+---------------------------------------------------------------------*/
static unsigned int
LoadAllSamples(const char* filename, unsigned int repeats)
{
    // open the input
    AP4_ByteStream* input = NULL;
    AP4_Result result = AP4_FileByteStream::Create(filename, AP4_FileByteStream::STREAM_MODE_READ, input);
    if (AP4_FAILED(result)) {
        fprintf(stderr, "ERROR: cannot open input file (%s)\n", filename);
        return 0;
    }
        
    // parse the file
    AP4_File* mp4_file = new AP4_File(*input);

    // create a stream decrypter if necessary
    if (mp4_file->GetFileType()->GetMajorBrand() == AP4_OMA_DCF_BRAND_ODCF) {
        const AP4_UI08 key[16] = {0x00, 0x01, 0x02, 0x03, 0x04, 0x05, 0x06, 0x07, 0x08, 0x09, 0x0a, 0x0b, 0x0c, 0x0d, 0x0e, 0x0f};
        input->Release();
        input = CreateDcfDecrypter(*mp4_file, key);
        delete mp4_file;
        mp4_file = new AP4_File(*input);
    }
    
    // dump all the tracks that need to be dumped
    unsigned int total_size = 0;
    for (AP4_List<AP4_Track>::Item* item = mp4_file->GetMovie()->GetTracks().FirstItem(); item; item=item->GetNext()) {
        AP4_Track* track = item->GetData();
        total_size += LoadSamples(track, repeats);
    }

    delete mp4_file;
    input->Release();
    
    return total_size;
}

/*----------------------------------------------------------------------
|   ReadFile
+---------------------------------------------------------------------*/
static unsigned int
ReadFile(const char* filename, unsigned int block_size, bool sequential)
{
    AP4_ByteStream* input = NULL;
    AP4_Result result = AP4_FileByteStream::Create(filename, AP4_FileByteStream::STREAM_MODE_READ, input);
    if (AP4_FAILED(result)) {
        fprintf(stderr, "ERROR: cannot open file %s (error %d)\n", filename, result);
        return 0;
    }
    
    AP4_LargeSize file_size = 0;
    input->GetSize(file_size);
    
    AP4_UI08* buffer = new AP4_UI08[block_size];
    unsigned int repeats = 128;
    unsigned int total_read = 0;
    AP4_Position position = 0;
    while (repeats--) {
        AP4_Result result;
        unsigned int blocks_to_read = (unsigned int)(file_size/block_size);
        if (blocks_to_read > 4096) blocks_to_read = 4096;
        input->Seek(0);
        while (blocks_to_read--) {
            AP4_Size bytes_read = 0;
            result = input->ReadPartial(buffer, block_size, bytes_read);
            if (AP4_FAILED(result)) {
                if (result == AP4_ERROR_EOS) break;
                fprintf(stderr, "ERROR: read failed\n");
                return 0;
            } else {
                total_read += bytes_read;
            }
            if (!sequential) {
                position += file_size-(file_size/7);
                position %= file_size;
                input->Seek(position); 
            }
        }
    }
    
    delete[] buffer;
    input->Release();
    
    return total_read;
}

/*----------------------------------------------------------------------
|   main
+---------------------------------------------------------------------*/
int
main(int argc, char** argv)
{
    if (argc < 2) {
        PrintUsage();
        return 1;
    }
    
    bool do_aes_cbc_block_decrypt  = false;
    bool do_aes_cbc_block_encrypt  = false;
    bool do_aes_ctr_block          = false;
    bool do_aes_cbc_stream_encrypt = false;
    bool do_aes_cbc_stream_decrypt = false;
    bool do_aes_ctr_stream         = false;
    bool do_read_file_seq_1        = false;
    bool do_read_file_seq_16       = false;
    bool do_read_file_seq_256      = false;
    bool do_read_file_seq_4096     = false;
    bool do_read_file_rnd_1        = false;
    bool do_read_file_rnd_16       = false;
    bool do_read_file_rnd_256      = false;
    bool do_read_file_rnd_4096     = false;
    bool do_parse_file             = false;
    bool do_parse_file_buffered    = false;
    bool do_parse_samples          = false;
    bool do_read_samples           = false;
    bool do_read_samples_dcf_cbc   = false;
    bool do_read_samples_dcf_ctr   = false;
    bool do_read_samples_pdcf_cbc  = false;
    bool do_read_samples_pdcf_ctr  = false;
    const char* test_file_read     = "test-bench.mp4";
    const char* test_file_mp4      = "test-bench.mp4";
    const char* test_file_dcf_cbc  = "test-bench.mp4.cbc.odf";
    const char* test_file_dcf_ctr  = "test-bench.mp4.ctr.odf";
    const char* test_file_pdcf_cbc = "test-bench.cbc.pdcf.mp4";
    const char* test_file_pdcf_ctr = "test-bench.ctr.pdcf.mp4";
    float max_time = TIME_SPAN;
    unsigned int max_iterations = 0xFFFFFFFF;
    
    while (const char* arg = *(++argv)) {
        if (!strcmp(arg, "aes-cbc-block-decrypt")) {
            do_aes_cbc_block_decrypt = true;
        } else if (!strcmp(arg, "aes-cbc-block-encrypt")) {
            do_aes_cbc_block_encrypt = true;
        } else if (!strcmp(arg, "aes-ctr-block")) {
            do_aes_ctr_block = true;
        } else if (!strcmp(arg, "aes-cbc-stream-encrypt")) {
            do_aes_cbc_stream_encrypt = true;
        } else if (!strcmp(arg, "aes-cbc-stream-decrypt")) {
            do_aes_cbc_stream_decrypt = true;
        } else if (!strcmp(arg, "aes-ctr-stream")) {
            do_aes_ctr_stream = true;
        } else if (!strcmp(arg, "read-file-seq-1")) {
            do_read_file_seq_1 = true;
        } else if (!strcmp(arg, "read-file-seq-16")) {
            do_read_file_seq_16 = true;
        } else if (!strcmp(arg, "read-file-seq-256")) {
            do_read_file_seq_256 = true;
        } else if (!strcmp(arg, "read-file-seq-4096")) {
            do_read_file_seq_4096 = true;
        } else if (!strcmp(arg, "read-file-rnd-1")) {
            do_read_file_rnd_1 = true;
        } else if (!strcmp(arg, "read-file-rnd-16")) {
            do_read_file_rnd_16 = true;
        } else if (!strcmp(arg, "read-file-rnd-256")) {
            do_read_file_rnd_256 = true;
        } else if (!strcmp(arg, "read-file-rnd-4096")) {
            do_read_file_rnd_4096 = true;
        } else if (!strcmp(arg, "parse-file")) {
            do_parse_file = true;
        } else if (!strcmp(arg, "parse-file-buffered")) {
            do_parse_file_buffered = true;
        } else if (!strcmp(arg, "parse-samples")) {
            do_parse_samples = true;
        } else if (!strcmp(arg, "read-samples")) {
            do_read_samples = true;
        } else if (!strcmp(arg, "read-samples-dcf-cbc")) {
            do_read_samples_dcf_cbc = true;
        } else if (!strcmp(arg, "read-samples-dcf-ctr")) {
            do_read_samples_dcf_ctr = true;
        } else if (!strcmp(arg, "read-samples-pdcf-cbc")) {
            do_read_samples_pdcf_cbc = true;
        } else if (!strcmp(arg, "read-samples-pdcf-ctr")) {
            do_read_samples_pdcf_ctr = true;
        } else if (!strncmp(arg, "--test-file-read=", 17)) {
            test_file_read = arg+17;
        } else if (!strncmp(arg, "--test-file-mp4=", 16)) {
            test_file_mp4 = arg+16;
        } else if (!strncmp(arg, "--test-file-dcf-cbc=", 20)) {
            test_file_dcf_cbc = arg+20;
        } else if (!strncmp(arg, "--test-file-dcf-ctr=", 20)) {
            test_file_dcf_ctr = arg+20;
        } else if (!strncmp(arg, "--test-file-pdcf-cbc=", 21)) {
            test_file_pdcf_cbc = arg+21;
        } else if (!strncmp(arg, "--test-file-pdcf-ctr=", 21)) {
            test_file_pdcf_ctr = arg+21;
        } else if (!strncmp(arg, "--iterations=", 13)) {
            max_iterations = (unsigned int)strtoul(arg+13, NULL, 10);
        } else if (!strcmp(arg, "all")) {
            do_aes_cbc_block_decrypt  = true;
            do_aes_cbc_block_encrypt  = true;
            do_aes_ctr_block          = true;
            do_aes_cbc_stream_encrypt = true;
            do_aes_cbc_stream_decrypt = true;
            do_aes_ctr_stream         = true;
            do_read_file_seq_1        = true;
            do_read_file_seq_16       = true;
            do_read_file_seq_256      = true;
            do_read_file_seq_4096     = true;
            do_read_file_rnd_1        = true;
            do_read_file_rnd_16       = true;
            do_read_file_rnd_256      = true;
            do_read_file_rnd_4096     = true;
            do_parse_file             = true;
            do_parse_file_buffered    = true;
            do_parse_samples          = true;
            do_read_samples           = true;
            do_read_samples_dcf_cbc   = true;
            do_read_samples_dcf_ctr   = true;
            do_read_samples_pdcf_cbc  = true;
            do_read_samples_pdcf_ctr  = true;
        } else {
            fprintf(stderr, "ERROR: unknown test name (%s)\n", arg);
            return 1;
        }
    }
    
    unsigned char key[] = {
      0xc4, 0x56, 0x09, 0xfb, 0xe6, 0xa5, 0xde, 0xfd, 0xb0, 0x23, 0x10, 0x06,
      0x08, 0xbf, 0x3e, 0xbd
    };
    
    const int blocks_size = 32768;
    unsigned char blocks_in[blocks_size];
    AP4_SetMemory(blocks_in, 0, sizeof(blocks_in));
    unsigned char blocks_out[blocks_size];
    unsigned char* megabyte_in = new unsigned char[ENC_IN_BUFFER_SIZE];
    AP4_SetMemory(megabyte_in, 0, ENC_IN_BUFFER_SIZE);
    unsigned char* megabyte_out = new unsigned char[ENC_OUT_BUFFER_SIZE];
    
    AP4_BlockCipher* e_cbc_block_cipher;
    AP4_DefaultBlockCipherFactory::Instance.CreateCipher(AP4_BlockCipher::AES_128, AP4_BlockCipher::ENCRYPT, AP4_BlockCipher::CBC, NULL, key, 16, e_cbc_block_cipher);
    AP4_BlockCipher* d_cbc_block_cipher;
    AP4_DefaultBlockCipherFactory::Instance.CreateCipher(AP4_BlockCipher::AES_128, AP4_BlockCipher::DECRYPT, AP4_BlockCipher::CBC, NULL, key, 16, d_cbc_block_cipher);
    AP4_BlockCipher* ctr_block_cipher;
    AP4_DefaultBlockCipherFactory::Instance.CreateCipher(AP4_BlockCipher::AES_128, AP4_BlockCipher::ENCRYPT, AP4_BlockCipher::CTR, NULL, key, 16, ctr_block_cipher);

    AP4_CbcStreamCipher e_cbc_stream_cipher(e_cbc_block_cipher);
    AP4_CbcStreamCipher d_cbc_stream_cipher(d_cbc_block_cipher);
    AP4_CtrStreamCipher ctr_stream_cipher(ctr_block_cipher, 16);

    BENCH_START("AES CBC Block Encryption", do_aes_cbc_block_encrypt)
    for (unsigned b=0; b<256; b++) {
        e_cbc_block_cipher->Process(blocks_in, blocks_size, blocks_out, NULL);
    }
    total += 256*blocks_size;
    BENCH_END("MB", SCALE_MB)
    
    BENCH_START("AES CBC Block Decryption", do_aes_cbc_block_decrypt)
    for (unsigned b=0; b<256; b++) {
        d_cbc_block_cipher->Process(blocks_in, blocks_size, blocks_out, NULL);
    }
    total += 256*blocks_size;
    BENCH_END("MB", SCALE_MB)
         
    BENCH_START("AES CTR Block Encryption/Decryption", do_aes_ctr_block)
    for (unsigned b=0; b<256; b++) {
        ctr_block_cipher->Process(blocks_in, blocks_size, blocks_out, NULL);
    }
    total += 256*blocks_size;
    BENCH_END("MB", SCALE_MB)

    BENCH_START("AES CBC Stream Encryption", do_aes_cbc_stream_encrypt)
    AP4_Size out_size = ENC_OUT_BUFFER_SIZE;
    AP4_Result result = e_cbc_stream_cipher.ProcessBuffer(megabyte_in, ENC_IN_BUFFER_SIZE, megabyte_out, &out_size, false);
    if (AP4_FAILED(result)) fprintf(stderr, "ERROR\n");
    total += ENC_IN_BUFFER_SIZE;
    BENCH_END("MB", SCALE_MB)

    BENCH_START("AES CBC Stream Decryption", do_aes_cbc_stream_decrypt)
    AP4_Size out_size = ENC_OUT_BUFFER_SIZE;
    d_cbc_stream_cipher.ProcessBuffer(megabyte_in,ENC_IN_BUFFER_SIZE, megabyte_out, &out_size, false);
    total += ENC_IN_BUFFER_SIZE;
    BENCH_END("MB", SCALE_MB)

    BENCH_START("AES CTR Stream", do_aes_ctr_stream)
    AP4_Size out_size = ENC_OUT_BUFFER_SIZE;
    ctr_stream_cipher.ProcessBuffer(megabyte_in, ENC_IN_BUFFER_SIZE, megabyte_out, &out_size, false);
    total += ENC_IN_BUFFER_SIZE;
    BENCH_END("MB", SCALE_MB)

    BENCH_START("Read File Sequential (1 Byte Blocks)", do_read_file_seq_1)
    total += ReadFile(test_file_read, 1, true);
    BENCH_END("MB", SCALE_MB)

    BENCH_START("Read File Sequential (16 Byte Blocks)", do_read_file_seq_16)
    total += ReadFile(test_file_read, 16, true);
    BENCH_END("MB", SCALE_MB)

    BENCH_START("Read File Sequential (256 Byte Blocks)", do_read_file_seq_256)
    total += ReadFile(test_file_read, 256, true);
    BENCH_END("MB", SCALE_MB)

    BENCH_START("Read File Sequential (4096 Byte Blocks)", do_read_file_seq_4096)
    total += ReadFile(test_file_read, 4096, true);
    BENCH_END("MB", SCALE_MB)

    BENCH_START("Read File Random (1 Byte Blocks)", do_read_file_rnd_1)
    total += ReadFile(test_file_read, 1, false);
    BENCH_END("MB", SCALE_MB)

    BENCH_START("Read File Random (16 Byte Blocks)", do_read_file_rnd_16)
    total += ReadFile(test_file_read, 16, false);
    BENCH_END("MB", SCALE_MB)

    BENCH_START("Read File Random (256 Byte Blocks)", do_read_file_rnd_256)
    total += ReadFile(test_file_read, 256, false);
    BENCH_END("MB", SCALE_MB)

    BENCH_START("Read File Random (4096 Byte Blocks)", do_read_file_rnd_4096)
    total += ReadFile(test_file_read, 4096, false);
    BENCH_END("MB", SCALE_MB)

    BENCH_START("Parse File", do_parse_file)
    total += ParseFile(test_file_mp4, 10, false);
    BENCH_END("files", 1)

    BENCH_START("Parse File Buffered", do_parse_file_buffered)
    total += ParseFile(test_file_mp4, 10, true);
    BENCH_END("MB", SCALE_MB)

    BENCH_START("Parse Samples", do_parse_samples)
    total += ParseAllSamples(test_file_mp4, 10);
    BENCH_END("samples", 1)

    BENCH_START("Read Samples", do_read_samples)
    total += LoadAllSamples(test_file_mp4, 16);
    BENCH_END("MB", SCALE_MB)

    BENCH_START("Read Samples DCF CBC", do_read_samples_dcf_cbc)
    total += LoadAllSamples(test_file_dcf_cbc, 16);
    BENCH_END("MB", SCALE_MB)

    BENCH_START("Read Samples DCF CTR", do_read_samples_dcf_ctr)
    total += LoadAllSamples(test_file_dcf_ctr, 16);
    BENCH_END("MB", SCALE_MB)

    BENCH_START("Read Samples PDCF CBC", do_read_samples_pdcf_cbc)
    total += LoadAllSamples(test_file_pdcf_cbc, 16);
    BENCH_END("MB", SCALE_MB)

    BENCH_START("Read Samples PDCF CTR", do_read_samples_pdcf_ctr)
    total += LoadAllSamples(test_file_pdcf_ctr, 16);
    BENCH_END("MB", SCALE_MB)

    return 1;
}
