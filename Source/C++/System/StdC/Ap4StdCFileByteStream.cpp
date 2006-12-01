/*****************************************************************
|
|      File Byte Stream
|
|      (c) 2001-2002 Gilles Boccon-Gibod
|
|
 ****************************************************************/

/*----------------------------------------------------------------------
|   includes
+---------------------------------------------------------------------*/
#include <stdio.h>
#include <string.h>
#if !defined(UNDER_CE)
#include <errno.h>
#include <sys/stat.h>
#endif

#include "Ap4FileByteStream.h"

/*----------------------------------------------------------------------
|   compatibility wrappers
+---------------------------------------------------------------------*/
#if !defined(ENOENT)
#define ENOENT 2
#endif
#if !defined(EACCES)
#define EACCES 13
#endif

#if !defined(AP4_CONFIG_HAVE_FOPEN_S)
static int fopen_s(FILE**      file,
                   const char* filename,
                   const char* mode)
{
    *file = fopen(filename, mode);
#if defined(UNDER_CE)
    if (*file == NULL) return ENOENT;
#else
    if (*file == NULL) return errno;
#endif
    return 0;
}
#endif // defined(AP4_CONFIG_HAVE_FOPEN_S

/*----------------------------------------------------------------------
|   AP4_StdcFileByteStream
+---------------------------------------------------------------------*/
class AP4_StdcFileByteStream: public AP4_ByteStream
{
public:
    // methods
    AP4_StdcFileByteStream(AP4_ByteStream*          delegator,
                           const char*              name, 
                           AP4_FileByteStream::Mode mode);
    ~AP4_StdcFileByteStream();

    // AP4_ByteStream methods
    AP4_Result Read(void*    buffer, 
                   AP4_Size  bytesToRead, 
                   AP4_Size* bytesRead);
    AP4_Result Write(const void* buffer, 
                    AP4_Size     bytesToWrite, 
                    AP4_Size*    bytesWritten);
    AP4_Result Seek(AP4_Offset offset);
    AP4_Result Tell(AP4_Offset& offset);
    AP4_Result GetSize(AP4_Size& size);

    // AP4_Referenceable methods
    void AddReference();
    void Release();

private:
    // members
    AP4_ByteStream* m_Delegator;
    AP4_Cardinal    m_ReferenceCount;
    FILE*           m_File;
    AP4_Size        m_Size;
};

/*----------------------------------------------------------------------
|   AP4_StdcFileByteStream::AP4_StdcFileByteStream
+---------------------------------------------------------------------*/
AP4_StdcFileByteStream::AP4_StdcFileByteStream(
    AP4_ByteStream  *        delegator,
    const char*              name, 
    AP4_FileByteStream::Mode mode) :
    m_Delegator(delegator),
    m_ReferenceCount(1),
    m_File(NULL),
    m_Size(0)
{
    if (!strcmp(name, "-stdin")) {
        m_File = stdin;
    } else if (!strcmp(name, "-stdout")) {
        m_File = stdout;
    } else if (!strcmp(name, "-stderr")) {
        m_File = stderr;
    } else {
        int open_result;
        switch (mode) {
          case AP4_FileByteStream::STREAM_MODE_READ:
            open_result = fopen_s(&m_File, name, "rb");
            break;

          case AP4_FileByteStream::STREAM_MODE_WRITE:
            open_result = fopen_s(&m_File, name, "wb+");
            break;

          default:
            throw AP4_Exception(AP4_ERROR_INVALID_PARAMETERS);
        }
    
        if (open_result != 0) {
            if (open_result == ENOENT) {
                throw AP4_Exception(AP4_ERROR_NO_SUCH_FILE);
            } else if (open_result == EACCES) {
                throw AP4_Exception(AP4_ERROR_PERMISSION_DENIED);
            } else {
                throw AP4_Exception(AP4_ERROR_CANNOT_OPEN_FILE);
            }
        }

        // get the size
        if (fseek(m_File, 0, SEEK_END) >= 0) {
            m_Size = ftell(m_File);
            fseek(m_File, 0, SEEK_SET);
        }
    }
}

/*----------------------------------------------------------------------
|   AP4_StdcFileByteStream::~AP4_StdcFileByteStream
+---------------------------------------------------------------------*/
AP4_StdcFileByteStream::~AP4_StdcFileByteStream()
{
    if (m_File && m_File != stdin && m_File != stdout && m_File != stderr) {
        fclose(m_File);
    }
}

/*----------------------------------------------------------------------
|   AP4_StdcFileByteStream::AddReference
+---------------------------------------------------------------------*/
void
AP4_StdcFileByteStream::AddReference()
{
    m_ReferenceCount++;
}

/*----------------------------------------------------------------------
|   AP4_StdcFileByteStream::Release
+---------------------------------------------------------------------*/
void
AP4_StdcFileByteStream::Release()
{
    if (--m_ReferenceCount == 0) {
        delete m_Delegator;
    }
}

/*----------------------------------------------------------------------
|   AP4_StdcFileByteStream::Read
+---------------------------------------------------------------------*/
AP4_Result
AP4_StdcFileByteStream::Read(void*    buffer, 
                            AP4_Size  bytesToRead, 
                            AP4_Size* bytesRead)
{
    size_t nbRead;

    nbRead = fread(buffer, 1, bytesToRead, m_File);

    if (nbRead > 0) {
        if (bytesRead) *bytesRead = (AP4_Size)nbRead;
        return AP4_SUCCESS;
    } else if (feof(m_File)) {
        if (bytesRead) *bytesRead = 0;
        return AP4_ERROR_EOS;
    } else {
        if (bytesRead) *bytesRead = 0;
        return AP4_ERROR_READ_FAILED;
    }
}

/*----------------------------------------------------------------------
|   AP4_StdcFileByteStream::Write
+---------------------------------------------------------------------*/
AP4_Result
AP4_StdcFileByteStream::Write(const void* buffer, 
                             AP4_Size     bytesToWrite, 
                             AP4_Size*    bytesWritten)
{
    size_t nbWritten;

    if (bytesToWrite == 0) return AP4_SUCCESS;
    nbWritten = fwrite(buffer, 1, bytesToWrite, m_File);
    
    if (nbWritten > 0) {
        if (bytesWritten) *bytesWritten = (AP4_Size)nbWritten;
        return AP4_SUCCESS;
    } else {
        if (bytesWritten) *bytesWritten = 0;
        return AP4_ERROR_WRITE_FAILED;
    }
}

/*----------------------------------------------------------------------
|   AP4_StdcFileByteStream::Seek
+---------------------------------------------------------------------*/
AP4_Result
AP4_StdcFileByteStream::Seek(AP4_Offset offset)
{
    size_t result;

    result = fseek(m_File, offset, SEEK_SET);
    if (result == 0) {
        return AP4_SUCCESS;
    } else {
        return AP4_FAILURE;
    }
}

/*----------------------------------------------------------------------
|   AP4_StdcFileByteStream::Tell
+---------------------------------------------------------------------*/
AP4_Result
AP4_StdcFileByteStream::Tell(AP4_Offset& offset)
{
    offset = ftell(m_File);
    return AP4_SUCCESS;
}

/*----------------------------------------------------------------------
|   AP4_StdcFileByteStream::GetSize
+---------------------------------------------------------------------*/
AP4_Result
AP4_StdcFileByteStream::GetSize(AP4_Size& size)
{
    size = m_Size;
    return AP4_SUCCESS;
}

/*----------------------------------------------------------------------
|   AP4_FileByteStream::AP4_FileByteStream
+---------------------------------------------------------------------*/
AP4_FileByteStream::AP4_FileByteStream(const char*             name, 
                                     AP4_FileByteStream::Mode mode)
{
    m_Delegate = new AP4_StdcFileByteStream(this, name, mode);
}











