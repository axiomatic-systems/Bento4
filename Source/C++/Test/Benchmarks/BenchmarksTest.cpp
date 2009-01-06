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

/*----------------------------------------------------------------------
|   macros
+---------------------------------------------------------------------*/
#define BENCH_START(_msg, _select)      \
if (_select) {                          \
    double before = GetTime();          \
    double after;                       \
    double total = 0.0f;                \
    printf("%s:", _msg);                \
    fflush(stdout);                     \
    unsigned int i;                     \
    for (i=0; ((after=GetTime())-before)<TIME_SPAN; i++) { 

#define BENCH_END                                         \
    }                                                     \
    double time_diff = after-before;                      \
    double mb = total/(1024.0f*1024.0f);                  \
    double mbps = (mb/time_diff);                         \
    printf(" %f MB/s (%f MB in %f seconds, %d iterations)\n", mbps, mb, time_diff, i); \
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
           "  --test-file-1=<filename> (MP4 file for read-samples)\n"
           "  --test-file-2=<filename> (DCF file for read-samples-dcf)\n"
           "\n"
           "valid test names are:\n"
           "all: run all tests\n"
           "or one or more of the following tests:\n"
           "aes-block-decrypt, aes-block-encrypt, \n"
           "aes-cbc-stream-encrypt, aes-cbc-stream-decrypt\n"
           "aes-ctr-stream\n"
           "read-samples\n"
           "read-samples-dcf\n");
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
        AP4_ContainerAtom* odrm = dynamic_cast<AP4_ContainerAtom*>(atom);
        if (odrm == NULL) continue; // not enough info
        AP4_OdheAtom* odhe = dynamic_cast<AP4_OdheAtom*>(odrm->GetChild(AP4_ATOM_TYPE_ODHE));
        if (odhe == NULL) continue; // not enough info    
        AP4_OddaAtom* odda = dynamic_cast<AP4_OddaAtom*>(odrm->GetChild(AP4_ATOM_TYPE_ODDA));;
        if (odda == NULL) continue; // not enough info
        AP4_OhdrAtom* ohdr = dynamic_cast<AP4_OhdrAtom*>(odhe->GetChild(AP4_ATOM_TYPE_OHDR));
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
|   LoadSamples
+---------------------------------------------------------------------*/
static unsigned int
LoadSamples(AP4_Track* track, unsigned int repeat)
{
    unsigned int total_size = 0;
    while (repeat--) {
        AP4_Sample     sample;
        AP4_DataBuffer sample_data;
        AP4_Ordinal    index = 0;
        while (AP4_SUCCEEDED(track->ReadSample(index, sample, sample_data))) {
            total_size += sample.GetSize();
            index++;
        }
    }
    
    return total_size;
}

#if 0
/*----------------------------------------------------------------------
|   DecryptAndDumpSamples
+---------------------------------------------------------------------*/
static void
DecryptAndDumpSamples(AP4_Track*             track, 
                      AP4_SampleDescription* sample_desc,
                      const AP4_UI08*        key,
                      AP4_ByteStream*        dump)
{
    AP4_ProtectedSampleDescription* pdesc = 
        dynamic_cast<AP4_ProtectedSampleDescription*>(sample_desc);
    if (pdesc == NULL) {
        fprintf(stderr, "ERROR: unable to obtain cipher info\n");
        return;
    }

    // create the decrypter
    AP4_SampleDecrypter* decrypter = AP4_SampleDecrypter::Create(pdesc, key, 16);
    if (decrypter == NULL) {
        fprintf(stderr, "ERROR: unable to create decrypter\n");
        return;
    }

    AP4_Sample     sample;
    AP4_DataBuffer encrypted_data;
    AP4_DataBuffer decrypted_data;
    AP4_Ordinal    index = 0;
    while (AP4_SUCCEEDED(track->ReadSample(index, sample, encrypted_data))) {
        if (AP4_FAILED(decrypter->DecryptSampleData(encrypted_data, decrypted_data))) {
            fprintf(stderr, "ERROR: failed to decrypt sample\n");
            return;
        }

        // write the sample size
        dump->WriteUI32(decrypted_data.GetDataSize());

        // write the sample
        dump->Write(decrypted_data.GetData(), decrypted_data.GetDataSize());
        index++;
        if (index%10 == 0) printf(".");
    }
    printf("\n");
}
#endif

/*----------------------------------------------------------------------
|   LoadAllSamples
+---------------------------------------------------------------------*/
static unsigned int
LoadAllSamples(const char* filename, unsigned int repeats, bool is_dcf)
{
    // open the input
    AP4_ByteStream* input = NULL;
    try {
        input = new AP4_FileByteStream(filename,
                               AP4_FileByteStream::STREAM_MODE_READ);
    } catch (AP4_Exception) {
        fprintf(stderr, "ERROR: cannot open input file (%s)\n", filename);
        return 0;
    }
        
    // parse the file
    AP4_File* mp4_file = new AP4_File(*input);

    // create a stream decrypter if necessary
    if (is_dcf) {
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

        // get the sample description
        AP4_SampleDescription* sample_description = track->GetSampleDescription(0);

        switch(sample_description ?
               sample_description->GetType() :
               AP4_SampleDescription::TYPE_UNKNOWN) {
#if 0
            case AP4_SampleDescription::TYPE_PROTECTED:
                {
                    const AP4_UI08* key = key_map.GetKey(track_id);
                    if (key == NULL) {
                        fprintf(stderr, 
                                "WARNING: No key found for encrypted track %d... "
                                "dumping encrypted samples\n",
                                track_id);
                        DumpSamples(track, dump);
                    } else {
                        DecryptAndDumpSamples(track, sample_description, key, dump);
                    }
                }
                break;
#endif
            default:
                total_size += LoadSamples(track, repeats);
        
        }
    }

    delete mp4_file;
    input->Release();
    
    return total_size;
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
    
    bool do_aes_block_decrypt      = false;
    bool do_aes_block_encrypt      = false;
    bool do_aes_cbc_stream_encrypt = false;
    bool do_aes_cbc_stream_decrypt = false;
    bool do_aes_ctr_stream         = false;
    bool do_read_samples           = false;
    bool do_read_samples_dcf       = false;
    const char* test_file_1        = "test-001.mp4";
    const char* test_file_2        = "test-001.mp4.cbc.odf";
    
    while (const char* arg = *(++argv)) {
        if (!strcmp(arg, "aes-block-decrypt")) {
            do_aes_block_decrypt = true;
        } else if (!strcmp(arg, "aes-block-encrypt")) {
            do_aes_block_encrypt = true;
        } else if (!strcmp(arg, "aes-cbc-stream-encrypt")) {
            do_aes_cbc_stream_encrypt = true;
        } else if (!strcmp(arg, "aes-cbc-stream-decrypt")) {
            do_aes_cbc_stream_decrypt = true;
        } else if (!strcmp(arg, "aes-ctr-stream")) {
            do_aes_ctr_stream = true;
        } else if (!strcmp(arg, "read-samples")) {
            do_read_samples = true;
        } else if (!strcmp(arg, "read-samples-dcf")) {
            do_read_samples_dcf = true;
        } else if (!strncmp(arg, "--test-file-1=", 14)) {
            test_file_1 = arg+14;
        } else if (!strncmp(arg, "--test-file-2=", 14)) {
            test_file_2 = arg+14;
        } else if (!strcmp(arg, "all")) {
            do_aes_block_decrypt      = true;
            do_aes_block_encrypt      = true;
            do_aes_cbc_stream_encrypt = true;
            do_aes_cbc_stream_decrypt = true;
            do_aes_ctr_stream         = true;
            do_read_samples           = true;
        } else {
            fprintf(stderr, "ERROR: unknown test name (%s)\n", arg);
            return 1;
        }
    }
    
    unsigned char key[] = {
      0xc4, 0x56, 0x09, 0xfb, 0xe6, 0xa5, 0xde, 0xfd, 0xb0, 0x23, 0x10, 0x06,
      0x08, 0xbf, 0x3e, 0xbd
    };
    
    unsigned char block_in[16];
    AP4_SetMemory(block_in, 0, sizeof(block_in));
    unsigned char block_out[16];
    unsigned char* megabyte_in = new unsigned char[ENC_IN_BUFFER_SIZE];
    AP4_SetMemory(megabyte_in, 0, sizeof(megabyte_in));
    unsigned char* megabyte_out = new unsigned char[ENC_OUT_BUFFER_SIZE];
    
    AP4_BlockCipher* e_block_cipher;
    AP4_DefaultBlockCipherFactory::Instance.Create(AP4_BlockCipher::AES_128, AP4_BlockCipher::ENCRYPT, key, 16, e_block_cipher);
    AP4_BlockCipher* d_block_cipher;
    AP4_DefaultBlockCipherFactory::Instance.Create(AP4_BlockCipher::AES_128, AP4_BlockCipher::DECRYPT, key, 16, d_block_cipher);
    AP4_BlockCipher* g_block_cipher;
    AP4_DefaultBlockCipherFactory::Instance.Create(AP4_BlockCipher::AES_128, AP4_BlockCipher::DECRYPT, key, 16, g_block_cipher);

    AP4_CbcStreamCipher cbc_e_cipher(e_block_cipher, AP4_CbcStreamCipher::ENCRYPT);
    AP4_CbcStreamCipher cbc_d_cipher(d_block_cipher, AP4_CtrStreamCipher::DECRYPT);
    AP4_CtrStreamCipher ctr_cipher(g_block_cipher, NULL, 16);


    BENCH_START("AES Block Encryption", do_aes_block_encrypt)
    for (unsigned b=0; b<1024; b++) {
        e_block_cipher->ProcessBlock(block_in, block_out);
    }
    total += 1024*16;
    BENCH_END
    
    BENCH_START("AES Block Decryption", do_aes_block_decrypt)
    for (unsigned b=0; b<1024; b++) {
        d_block_cipher->ProcessBlock(block_in, block_out);
    }
    total += 1024*16;
    BENCH_END
         
    BENCH_START("AES CBC Stream Encryption", do_aes_cbc_stream_encrypt)
    AP4_Size out_size = ENC_OUT_BUFFER_SIZE;
    AP4_Result result = cbc_e_cipher.ProcessBuffer(megabyte_in, ENC_IN_BUFFER_SIZE, megabyte_out, &out_size, false);
    if (AP4_FAILED(result)) fprintf(stderr, "ERROR\n");
    total += ENC_IN_BUFFER_SIZE;
    BENCH_END

    BENCH_START("AES CBC Stream Decryption", do_aes_cbc_stream_decrypt)
    AP4_Size out_size = ENC_OUT_BUFFER_SIZE;
    cbc_d_cipher.ProcessBuffer(megabyte_in,ENC_IN_BUFFER_SIZE, megabyte_out, &out_size, false);
    total += ENC_IN_BUFFER_SIZE;
    BENCH_END

    BENCH_START("AES CTR Stream", do_aes_ctr_stream)
    AP4_Size out_size = ENC_OUT_BUFFER_SIZE;
    ctr_cipher.ProcessBuffer(megabyte_in, ENC_IN_BUFFER_SIZE, megabyte_out, &out_size, false);
    total += ENC_IN_BUFFER_SIZE;
    BENCH_END

    BENCH_START("Read Samples", do_read_samples)
    total += LoadAllSamples(test_file_1, 16, false);
    BENCH_END

    BENCH_START("Read Samples DCF", do_read_samples_dcf)
    total += LoadAllSamples(test_file_2, 16, true);
    BENCH_END

    return 1;
}
