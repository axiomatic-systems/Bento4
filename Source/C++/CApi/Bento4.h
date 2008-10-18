/*****************************************************************
|
|    Bento4 - C API Main Header
|
|    Copyright 2002-2008 Gilles Boccon-Gibod & Julien Boeuf
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
#include "Ap4Types.h"

/*----------------------------------------------------------------------
|   types
+---------------------------------------------------------------------*/
typedef struct AP4_ByteStream AP4_ByteStream;
typedef struct AP4_DataBuffer AP4_DataBuffer;
typedef struct AP4_File AP4_File;
typedef struct AP4_Movie AP4_Movie;
typedef struct AP4_Track AP4_Track;
typedef struct AP4_MetaData AP4_MetaData;
typedef struct AP4_Sample AP4_Sample;

/*----------------------------------------------------------------------
|   constants
+---------------------------------------------------------------------*/
extern const int AP4_FILE_BYTE_STREAM_MODE_READ;
extern const int AP4_FILE_BYTE_STREAM_MODE_WRITE;
extern const int AP4_FILE_BYTE_STREAM_MODE_READ_WRITE;

extern const int AP4_TRACK_TYPE_UNKNOWN;
extern const int AP4_TRACK_TYPE_AUDIO;
extern const int AP4_TRACK_TYPE_VIDEO;
extern const int AP4_TRACK_TYPE_SYSTEM;
extern const int AP4_TRACK_TYPE_HINT;
extern const int AP4_TRACK_TYPE_TEXT;
extern const int AP4_TRACK_TYPE_JPEG;
extern const int AP4_TRACK_TYPE_RTP;

extern const AP4_UI32 AP4_TRACK_DEFAULT_MOVIE_TIMESCALE;

/*----------------------------------------------------------------------
|   result codes
+---------------------------------------------------------------------*/
extern const int AP4_SUCCESS;
extern const int AP4_FAILURE;
extern const int AP4_ERROR_OUT_OF_MEMORY;
extern const int AP4_ERROR_INVALID_PARAMETERS;
extern const int AP4_ERROR_NO_SUCH_FILE;
extern const int AP4_ERROR_PERMISSION_DENIED;
extern const int AP4_ERROR_CANNOT_OPEN_FILE;
extern const int AP4_ERROR_EOS;
extern const int AP4_ERROR_WRITE_FAILED;
extern const int AP4_ERROR_READ_FAILED;
extern const int AP4_ERROR_INVALID_FORMAT;
extern const int AP4_ERROR_NO_SUCH_ITEM;
extern const int AP4_ERROR_OUT_OF_RANGE;
extern const int AP4_ERROR_INTERNAL;
extern const int AP4_ERROR_INVALID_STATE;
extern const int AP4_ERROR_LIST_EMPTY;
extern const int AP4_ERROR_LIST_OPERATION_ABORTED;
extern const int AP4_ERROR_INVALID_RTP_CONSTRUCTOR_TYPE;
extern const int AP4_ERROR_NOT_SUPPORTED;
extern const int AP4_ERROR_INVALID_TRACK_TYPE;
extern const int AP4_ERROR_INVALID_RTP_PACKET_EXTRA_DATA;
extern const int AP4_ERROR_BUFFER_TOO_SMALL;
extern const int AP4_ERROR_NOT_ENOUGH_DATA;

#ifdef __cplusplus
extern "C" {
#endif /* __cplusplus */

/*----------------------------------------------------------------------
|   AP4_ByteStream methods
+---------------------------------------------------------------------*/
void 
AP4_ByteStream_AddReference(AP4_ByteStream* self);

void 
AP4_ByteStream_Release(AP4_ByteStream* self);

AP4_Result 
AP4_ByteStream_ReadPartial(AP4_ByteStream*  self,
                           void*            buffer,
                           AP4_Size         bytes_to_read,
                           AP4_Size*        bytes_read);

AP4_Result 
AP4_ByteStream_Read(AP4_ByteStream* self, 
                    void*           buffer, 
                    AP4_Size        bytes_to_read);

AP4_Result 
AP4_ByteStream_ReadDouble(AP4_ByteStream* self, double* value);

AP4_Result 
AP4_ByteStream_ReadUI64(AP4_ByteStream* self, AP4_UI64* value);

AP4_Result 
AP4_ByteStream_ReadUI32(AP4_ByteStream* self, AP4_UI32* value);

AP4_Result 
AP4_ByteStream_ReadUI24(AP4_ByteStream* self, AP4_UI32* value);

AP4_Result 
AP4_ByteStream_ReadUI16(AP4_ByteStream* self, AP4_UI16* value);

AP4_Result 
AP4_ByteStream_ReadUI08(AP4_ByteStream* self, AP4_UI08* value);

AP4_Result 
AP4_ByteStream_ReadString(AP4_ByteStream* self, 
                          char*           buffer, 
                          AP4_Size        size);

AP4_Result 
AP4_ByteStream_WritePartial(AP4_ByteStream* self,
                            const void*     buffer,
                            AP4_Size        bytes_to_write,
                            AP4_Size*       bytes_written);

AP4_Result 
AP4_ByteStream_Write(AP4_ByteStream* self, 
                     const void*     buffer, 
                     AP4_Size        bytes_to_write);
AP4_Result 
AP4_ByteStream_WriteDouble(AP4_ByteStream* self, double value);

AP4_Result 
AP4_ByteStream_WriteUI64(AP4_ByteStream* self, AP4_UI64 value);

AP4_Result 
AP4_ByteStream_WriteUI32(AP4_ByteStream* self, AP4_UI32 value);

AP4_Result 
AP4_ByteStream_WriteUI24(AP4_ByteStream* self, AP4_UI32 value);

AP4_Result 
AP4_ByteStream_WriteUI16(AP4_ByteStream* self, AP4_UI16 value);

AP4_Result 
AP4_ByteStream_WriteUI08(AP4_ByteStream* self, AP4_UI08 value);

AP4_Result 
AP4_ByteStream_WriteString(AP4_ByteStream* self, const char* buffer);

AP4_Result 
AP4_ByteStream_Seek(AP4_ByteStream* self, AP4_Position position);

AP4_Result 
AP4_ByteStream_Tell(AP4_ByteStream* self, AP4_Position* position);

AP4_Result 
AP4_ByteStream_GetSize(AP4_ByteStream* self, AP4_LargeSize* size);

AP4_Result 
AP4_ByteStream_CopyTo(AP4_ByteStream* self, 
                      AP4_ByteStream* receiver, 
                      AP4_LargeSize   size); 

AP4_Result Ap4_ByteStream_Flush(AP4_ByteStream* self);

/*----------------------------------------------------------------------
|   AP4_ByteStream construtors
+---------------------------------------------------------------------*/
AP4_ByteStream* 
AP4_SubStream_Create(AP4_ByteStream* container, 
                     AP4_Position     position, 
                     AP4_LargeSize    size);
AP4_ByteStream* 
AP4_MemoryByteStream_Create(AP4_Size size);

AP4_ByteStream* 
AP4_MemoryByteStream_FromBuffer(const AP4_UI08* buffer, AP4_Size size);

AP4_ByteStream*
AP4_MemoryByteStream_AdaptDataBuffer(AP4_DataBuffer* buffer); /* data is read/written from the supplied data buffer */

AP4_ByteStream*
AP4_FileByteStream_Create(const char* name, int mode);

/*----------------------------------------------------------------------
|   AP4_DataBuffer methods
+---------------------------------------------------------------------*/
AP4_Result 
AP4_DataBuffer_SetBuffer(AP4_DataBuffer* self, AP4_Byte* buffer, AP4_Size size);

AP4_Result 
AP4_DataBuffer_SetBufferSize(AP4_DataBuffer* self, AP4_Size size);

AP4_Size 
AP4_DataBuffer_GetBufferSize(const AP4_DataBuffer* self);

const AP4_Byte*
AP4_DataBuffer_GetData(const AP4_DataBuffer* self);

AP4_Byte*
AP4_DataBuffer_UseData(AP4_DataBuffer* self);

AP4_Size
AP4_DataBuffer_GetDataSize(const AP4_DataBuffer* self);

AP4_Result
AP4_DataBuffer_SetDataSize(AP4_DataBuffer* self, AP4_Size size);

AP4_Result
AP4_DataBuffer_SetData(AP4_DataBuffer* self, 
                      const AP4_Byte*  data, 
                      AP4_Size         data_size);
                      
AP4_Result
AP4_DataBuffer_Reserve(AP4_DataBuffer* self, AP4_Size size);

/*----------------------------------------------------------------------
|   AP4_DataBuffer constructors
+---------------------------------------------------------------------*/
AP4_DataBuffer*
AP4_DataBuffer_Create(AP4_Size size);

AP4_DataBuffer*
AP4_DataBuffer_FromData(const void* data, AP4_Size data_size);

AP4_DataBuffer*
AP4_DataBuffer_Clone(const AP4_DataBuffer* other);

/*----------------------------------------------------------------------
|   AP4_File methods
+---------------------------------------------------------------------*/
AP4_Movie*
AP4_File_GetMovie(AP4_File* self);

/* TODO AP4_File_GetFileType */

AP4_Result 
AP4_File_SetFileType(AP4_File*    self,
                     AP4_UI32     major_brand,
                     AP4_UI32     minor_version,
                     AP4_UI32*    compatible_brands,
                     AP4_Cardinal compatible_brand_count);
                     
int
AP4_File_IsMoovBeforeMdat(const AP4_File* self);

/* TODO AP4_File_Inspect */

const AP4_MetaData*
AP4_File_GetMetaData(AP4_File* self);

/*----------------------------------------------------------------------
|   AP4_File constructors
+---------------------------------------------------------------------*/
AP4_File*
AP4_File_Create(AP4_Movie* movie);

AP4_File*
AP4_File_FromStream(AP4_ByteStream* stream, int moov_only);

/*----------------------------------------------------------------------
|   AP4_Movie methods
+---------------------------------------------------------------------*/
AP4_Cardinal
AP4_Movie_GetTrackCount(AP4_Movie* self);

AP4_Track*
AP4_Movie_GetTrackByIndex(AP4_Movie* self, AP4_Ordinal index);

AP4_Track*
AP4_Movie_GetTrackById(AP4_Movie* self, AP4_UI32 track_id);

AP4_Track*
AP4_Movie_GetTrackByType(AP4_Movie* self, int type, AP4_Ordinal index);

AP4_UI32
AP4_Movie_GetTimeScale(AP4_Movie* self);

AP4_UI32
AP4_Movie_GetDuration(AP4_Movie* self);

AP4_Duration
AP4_Movie_GetDurationMs(AP4_Movie* self);

/*----------------------------------------------------------------------
|   AP4_Movie constructors
+---------------------------------------------------------------------*/
AP4_Movie*
AP4_Movie_Create(AP4_UI32 time_scale);                          

/*----------------------------------------------------------------------
|   AP4_Track methods
+---------------------------------------------------------------------*/
int
AP4_Track_GetType(AP4_Track* self);

AP4_UI32
AP4_Track_GetHandlerType(AP4_Track* self);

AP4_UI32
AP4_Track_GetDuration(AP4_Track* self); /* timescale of the movie */

AP4_Duration
AP4_Track_GetDurationMs(AP4_Track* self);

AP4_Cardinal
AP4_Track_GetSampleCount(AP4_Track* self);

/** to be continued... */



#ifdef __cplusplus
}
#endif /*__cplusplus */
