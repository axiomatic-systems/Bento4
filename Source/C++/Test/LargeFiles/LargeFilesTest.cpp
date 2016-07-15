/*****************************************************************
|
|      File Test Program 3
|
|      (c) 2005-2016 Gilles Boccon-Gibod
|      Author: Gilles Boccon-Gibod (bok@bok.net)
|
****************************************************************/

/*----------------------------------------------------------------------
|       includes
+---------------------------------------------------------------------*/
#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include "Ap4.h"

/*----------------------------------------------------------------------
|   globals
+---------------------------------------------------------------------*/
const unsigned int BUFFER_SIZE = 8192;
const unsigned int MAX_RANDOM = 123;
typedef struct {
    AP4_Position  position;
    unsigned int  size;
    unsigned char fill_value;
} BufferInfo;
const AP4_UI64 TARGET_SIZE = 0x112345678;
AP4_Array<BufferInfo> Buffers;

#define CHECK(x) \
do {\
  if (!(x)) {\
    fprintf(stderr, "ERROR line %d\n", __LINE__);\
    return AP4_FAILURE;\
  }\
} while(0)\

/*----------------------------------------------------------------------
|   TestLargeFiles
+---------------------------------------------------------------------*/
static AP4_Result
TestLargeFiles(const char* filename)
{
    // create enough buffers to fill up to the target size
    AP4_UI64 total_size = 0;
    while (total_size < TARGET_SIZE) {
        unsigned int random = ((unsigned int)rand()) % MAX_RANDOM;
        unsigned int buffer_size = 4096-MAX_RANDOM/2+random;
        BufferInfo buffer_info;
        buffer_info.position = total_size;
        buffer_info.size = buffer_size;
        buffer_info.fill_value = ((unsigned int)rand())%256;
        Buffers.Append(buffer_info);
        
        total_size += buffer_size;
    }
    unsigned char* buffer = new unsigned char[BUFFER_SIZE];
    unsigned int progress = 0;
    
    // write random buffers
    printf("Writing sequential random-size buffers\n");
    AP4_ByteStream* output_stream = NULL;
    AP4_Result result = AP4_FileByteStream::Create(filename, AP4_FileByteStream::STREAM_MODE_WRITE, output_stream);
    CHECK(result == AP4_SUCCESS);
    
    for (unsigned int i=0; i<Buffers.ItemCount(); i++) {
        unsigned int new_progress = (100*i)/Buffers.ItemCount();
        if (new_progress != progress) {
            printf("\rProgress: %d%%", new_progress);
            fflush(stdout);
            progress = new_progress;
        }
        
        BufferInfo& buffer_info = Buffers[i];
        AP4_SetMemory(buffer, buffer_info.fill_value, buffer_info.size);
        
        
        AP4_Position cursor;
        result = output_stream->Tell(cursor);
        CHECK(result == AP4_SUCCESS);
        CHECK(cursor == buffer_info.position);
        
        result = output_stream->Write(buffer, buffer_info.size);
        CHECK(result == AP4_SUCCESS);
        
        if ((buffer_info.fill_value % 7) == 0) {
            output_stream->Flush();
        }
        
        result = output_stream->Tell(cursor);
        CHECK(result == AP4_SUCCESS);
        CHECK(cursor == buffer_info.position+buffer_info.size);
    }
    
    output_stream->Release();
    output_stream = NULL;
    
    // read random buffers
    printf("\nReading sequential random-size buffers\n");
    AP4_ByteStream* input_stream = NULL;
    result = AP4_FileByteStream::Create(filename, AP4_FileByteStream::STREAM_MODE_READ, input_stream);
    CHECK(result == AP4_SUCCESS);
    
    for (unsigned int i=0; i<Buffers.ItemCount(); i++) {
        unsigned int new_progress = (100*i)/Buffers.ItemCount();
        if (new_progress != progress) {
            printf("\rProgress: %d%%", new_progress);
            fflush(stdout);
            progress = new_progress;
        }

        BufferInfo& buffer_info = Buffers[i];

        AP4_Position cursor;
        result = input_stream->Tell(cursor);
        CHECK(result == AP4_SUCCESS);
        CHECK(cursor == buffer_info.position);
        
        result = input_stream->Read(buffer, buffer_info.size);
        CHECK(result == AP4_SUCCESS);
        
        for (unsigned int x=0; x<buffer_info.size; x++) {
            CHECK(buffer[x] == buffer_info.fill_value);
        }
        
        result = input_stream->Tell(cursor);
        CHECK(result == AP4_SUCCESS);
        CHECK(cursor == buffer_info.position+buffer_info.size);
    }
    input_stream->Release();
    input_stream = NULL;

    // read random buffers
    printf("\nReading random-access random-size buffers\n");
    result = AP4_FileByteStream::Create(filename, AP4_FileByteStream::STREAM_MODE_READ, input_stream);
    CHECK(result == AP4_SUCCESS);
    
    for (unsigned int i=0; i<Buffers.ItemCount()*5; i++) {
        unsigned int new_progress = (100*i)/(5*Buffers.ItemCount());
        if (new_progress != progress) {
            printf("\rProgress: %d%%", new_progress);
            fflush(stdout);
            progress = new_progress;
        }
    
        unsigned int buffer_index = ((unsigned int)rand())%Buffers.ItemCount();
        BufferInfo& buffer_info = Buffers[buffer_index];

        result = input_stream->Seek(buffer_info.position);
        CHECK(result == AP4_SUCCESS);

        AP4_Position cursor;
        result = input_stream->Tell(cursor);
        CHECK(result == AP4_SUCCESS);
        CHECK(cursor == buffer_info.position);
        
        result = input_stream->Read(buffer, buffer_info.size);
        CHECK(result == AP4_SUCCESS);
        
        for (unsigned int x=0; x<buffer_info.size; x++) {
            CHECK(buffer[x] == buffer_info.fill_value);
        }
        
        result = input_stream->Tell(cursor);
        CHECK(result == AP4_SUCCESS);
        CHECK(cursor == buffer_info.position+buffer_info.size);
    }

    printf("\nSUCCESS!\n");
    
    return AP4_SUCCESS;
}

/*----------------------------------------------------------------------
|   main
+---------------------------------------------------------------------*/
int
main(int argc, char** argv)
{
    const char* output_filename = "largefile.bin";
    if (argc > 1) {
        output_filename = argv[1];
    }

    TestLargeFiles(output_filename);
    
    return 0;
}
