/*****************************************************************
|
|    AP4 - Utilities
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
#include "Ap4Utils.h"

/*----------------------------------------------------------------------
|   AP4_GlobalOptions::g_Entry
+---------------------------------------------------------------------*/
AP4_List<AP4_GlobalOptions::Entry>* AP4_GlobalOptions::g_Entries = NULL;

/*----------------------------------------------------------------------
|   AP4_GlobalOptions::GetEntry
+---------------------------------------------------------------------*/
AP4_GlobalOptions::Entry*
AP4_GlobalOptions::GetEntry(const char* name, bool autocreate)
{
    if (g_Entries == NULL) {
        g_Entries = new AP4_List<Entry>;
    }
    for (AP4_List<Entry>::Item* item = g_Entries->FirstItem();
                                item;
                                item = item->GetNext()) {
        if (item->GetData()->m_Name == name) return item->GetData();
    }
    
    if (autocreate) {
        Entry* new_entry = new Entry();
        new_entry->m_Name = name;
        g_Entries->Add(new_entry);
        return new_entry;
    } else {
        return NULL;
    }
}

/*----------------------------------------------------------------------
|   AP4_GlobalOptions::SetBool
+---------------------------------------------------------------------*/
void
AP4_GlobalOptions::SetBool(const char* name, bool value)
{
    Entry* entry = GetEntry(name, true);
    entry->m_Value = value?"true":"false";
}

/*----------------------------------------------------------------------
|   AP4_GlobalOptions::GetBool
+---------------------------------------------------------------------*/
bool
AP4_GlobalOptions::GetBool(const char* name)
{
    Entry* entry = GetEntry(name, false);
    if (entry) {
        return entry->m_Value == "true";
    } else {
        return false;
    }
}

/*----------------------------------------------------------------------
|   AP4_GlobalOptions::SetString
+---------------------------------------------------------------------*/
void
AP4_GlobalOptions::SetString(const char* name, const char* value)
{
    Entry* entry = GetEntry(name, true);
    entry->m_Value = value;
}

/*----------------------------------------------------------------------
|   AP4_GlobalOptions::GetString
+---------------------------------------------------------------------*/
const char*
AP4_GlobalOptions::GetString(const char* name)
{
    Entry* entry = GetEntry(name, false);
    if (entry) {
        return entry->m_Value.GetChars();
    } else {
        return NULL;
    }
}

/*----------------------------------------------------------------------
|   AP4_BytesToDoubleBE
+---------------------------------------------------------------------*/
double
AP4_BytesToDoubleBE(const unsigned char* bytes)
{
    AP4_UI64 i_value = AP4_BytesToUInt64BE(bytes);
    void*    v_value = reinterpret_cast<void*>(&i_value);
    double*  d_value = reinterpret_cast<double*>(v_value);
    
    return *d_value;
}

/*----------------------------------------------------------------------
|   AP4_BytesToUInt64BE
+---------------------------------------------------------------------*/
AP4_UI64
AP4_BytesToUInt64BE(const unsigned char* bytes)
{
    return 
        ( ((AP4_UI64)bytes[0])<<56 ) |
        ( ((AP4_UI64)bytes[1])<<48 ) |
        ( ((AP4_UI64)bytes[2])<<40 ) |
        ( ((AP4_UI64)bytes[3])<<32 ) |
        ( ((AP4_UI64)bytes[4])<<24 ) |
        ( ((AP4_UI64)bytes[5])<<16 ) |
        ( ((AP4_UI64)bytes[6])<<8  ) |
        ( ((AP4_UI64)bytes[7])     );    
}

/*----------------------------------------------------------------------
|   AP4_BytesFromDoubleBE
+---------------------------------------------------------------------*/
void
AP4_BytesFromDoubleBE(unsigned char* bytes, double value)
{
    void*     v_value = reinterpret_cast<void*>(&value);
    AP4_UI64* i_value = reinterpret_cast<AP4_UI64*>(v_value);
    
    AP4_BytesFromUInt64BE(bytes, *i_value);
}

/*----------------------------------------------------------------------
|   AP4_BytesFromUInt64BE
+---------------------------------------------------------------------*/
void
AP4_BytesFromUInt64BE(unsigned char* bytes, AP4_UI64 value)
{
    bytes[0] = (unsigned char)((value >> 56) & 0xFF);
    bytes[1] = (unsigned char)((value >> 48) & 0xFF);
    bytes[2] = (unsigned char)((value >> 40) & 0xFF);
    bytes[3] = (unsigned char)((value >> 32) & 0xFF);
    bytes[4] = (unsigned char)((value >> 24) & 0xFF);
    bytes[5] = (unsigned char)((value >> 16) & 0xFF);
    bytes[6] = (unsigned char)((value >>  8) & 0xFF);
    bytes[7] = (unsigned char)((value      ) & 0xFF);
}

/*----------------------------------------------------------------------
|   AP4_DurationMsFromUnits
+---------------------------------------------------------------------*/
AP4_UI32
AP4_DurationMsFromUnits(AP4_UI64 units, AP4_UI32 units_per_second)
{
	if (units_per_second == 0) return 0;
	return (AP4_UI32)(((double)units*1000.0)/(double)units_per_second);
}

/*----------------------------------------------------------------------
|   AP4_ConvertTime
+---------------------------------------------------------------------*/
AP4_UI64 
AP4_ConvertTime(AP4_UI64 time_value,
                AP4_UI32 from_time_scale,
                AP4_UI32 to_time_scale)
{
    if (from_time_scale == 0) return 0;
    double ratio = (double)to_time_scale/(double)from_time_scale;
    return ((AP4_UI64)(0.5+(double)time_value*ratio));
}

/*----------------------------------------------------------------------
|   AP4_FormatFourChars
+---------------------------------------------------------------------*/
void
AP4_FormatFourChars(char* str, AP4_UI32 value) {
    str[0] = (value >> 24) & 0xFF;
    str[1] = (value >> 16) & 0xFF;
    str[2] = (value >>  8) & 0xFF;
    str[3] = (value      ) & 0xFF;
    str[4] = '\0';
}

/*----------------------------------------------------------------------
|   AP4_FormatFourCharsPrintable
+---------------------------------------------------------------------*/
void
AP4_FormatFourCharsPrintable(char* str, AP4_UI32 value) {
    AP4_FormatFourChars(str, value);
    for (int i=0; i<4; i++) {
        if (str[i]<' ' || str[i] >= 127) {
            str[i] = '.';
        }
    }
}

/*----------------------------------------------------------------------
|   AP4_SplitArgs
+---------------------------------------------------------------------*/
AP4_Result
AP4_SplitArgs(char* arg, char*& arg0, char*& arg1)
{
    arg0 = arg;
    char* c = arg;
    while (*c != 0 && *c != ':') {
        c++;
    }
    if (*c == ':') {
        *c++ = '\0';
        arg1 = c;
        return AP4_SUCCESS;
    } else {
        return AP4_FAILURE;
    }
}

/*----------------------------------------------------------------------
|   AP4_SplitArgs
+---------------------------------------------------------------------*/
AP4_Result
AP4_SplitArgs(char* arg, char*& arg0, char*& arg1, char*& arg2)
{
    AP4_Result result = AP4_SplitArgs(arg, arg0, arg1);
    if (AP4_FAILED(result)) return result;
    return AP4_SplitArgs(arg1, arg1, arg2);
}

/*----------------------------------------------------------------------
|   AP4_HexNibble
+---------------------------------------------------------------------*/
unsigned char
AP4_HexNibble(char c)
{
    switch (c) {
        case '0': return 0;
        case '1': return 1;
        case '2': return 2;
        case '3': return 3;
        case '4': return 4;
        case '5': return 5;
        case '6': return 6;
        case '7': return 7;
        case '8': return 8;
        case '9': return 9;
        case 'a': case 'A': return 10;
        case 'b': case 'B': return 11;
        case 'c': case 'C': return 12;
        case 'd': case 'D': return 13;
        case 'e': case 'E': return 14;
        case 'f': case 'F': return 15;
        default: return 0;
    }
}

/*----------------------------------------------------------------------
|   AP4_NibbleHex
+---------------------------------------------------------------------*/
char
AP4_NibbleHex(unsigned int nibble) 
{
    if (nibble < 10) {
        return (char)('0'+nibble);
    } else if (nibble < 16) {
        return (char)('A'+(nibble-10));
    } else {
        return ' ';
    }
}

/*----------------------------------------------------------------------
|   AP4_ParseHex
+---------------------------------------------------------------------*/
AP4_Result
AP4_ParseHex(const char* hex, unsigned char* bytes, unsigned int count)
{
    if (AP4_StringLength(hex) < 2*count) return AP4_ERROR_INVALID_PARAMETERS;
    for (unsigned int i=0; i<count; i++) {
        bytes[i] = (AP4_HexNibble(hex[2*i]) << 4) | (AP4_HexNibble(hex[2*i+1]));
    }
    return AP4_SUCCESS;
}

/*----------------------------------------------------------------------
|   AP4_FormatHex
+---------------------------------------------------------------------*/
AP4_Result
AP4_FormatHex(const AP4_UI08* data, unsigned int data_size, char* hex)
{
    for (unsigned int i=0; i<data_size; i++) {
        *hex++ = AP4_NibbleHex(data[i]>>4);
        *hex++ = AP4_NibbleHex(data[i]&0x0F);
    }
    
    return AP4_SUCCESS;
}

/*----------------------------------------------------------------------
|   AP4_BitWriter::Write
+---------------------------------------------------------------------*/
void 
AP4_BitWriter::Write(AP4_UI32 bits, unsigned int bit_count)
{
    unsigned char* data = m_Data;
    if (m_BitCount+bit_count > m_DataSize*8) return;
    data += m_BitCount/8;
    unsigned int space = 8-(m_BitCount%8);
    while (bit_count) {
        unsigned int mask = bit_count==32 ? 0xFFFFFFFF : ((1<<bit_count)-1);
        if (bit_count <= space) {
            *data |= ((bits&mask) << (space-bit_count));
            m_BitCount += bit_count;
            return;
        } else {
            *data |= ((bits&mask) >> (bit_count-space));
            ++data;
            m_BitCount += space;
            bit_count  -= space;
            space       = 8;
        }
    }
}

/*----------------------------------------------------------------------
|   AP4_ParseIntegerU
+---------------------------------------------------------------------*/
AP4_UI32
AP4_ParseIntegerU(const char* str)
{
    if (str == NULL) {
        return 0;
    }

    // parse the digits
    AP4_UI32 value = 0;
    while (char c = *str++) {
        if (c >= '0' && c <= '9') {
            value = 10*value + (c-'0');
        } else {
            return 0;
        } 
    }

    return value;
}

/*----------------------------------------------------------------------
|   types and macros
+---------------------------------------------------------------------*/
#define AP4_WORD_BITS  32
#define AP4_WORD_BYTES 4

#define AP4_BIT_MASK(_n) ((1<<(_n))-1)

#if AP4_WORD_BITS != 32
#error unsupported word size /* 64 and other word size not yet implemented */
#endif

/*----------------------------------------------------------------------
|   AP4_BitReader::AP4_BitReader
+---------------------------------------------------------------------*/
AP4_BitReader::AP4_BitReader(const AP4_UI08* data, unsigned int data_size) :
    m_Position(0),
    m_Cache(0),
    m_BitsCached(0)
{
    // make the buffer an integral mulitple of the word size
    m_Buffer.SetBufferSize(AP4_WORD_BYTES*((data_size+AP4_WORD_BYTES-1)/AP4_WORD_BYTES));
    m_Buffer.SetData(data, data_size);
    if (m_Buffer.GetDataSize() != m_Buffer.GetBufferSize()) {
        AP4_SetMemory(m_Buffer.UseData()+m_Buffer.GetDataSize(), 0, m_Buffer.GetBufferSize()-m_Buffer.GetDataSize());
    }
}

/*----------------------------------------------------------------------
|   AP4_BitReader::~AP4_BitReader
+---------------------------------------------------------------------*/
AP4_BitReader::~AP4_BitReader()
{
}

/*----------------------------------------------------------------------
|   AP4_BitReader::Reset
+---------------------------------------------------------------------*/
AP4_Result
AP4_BitReader::Reset()
{
    m_Position   = 0;
    m_Cache      = 0;
    m_BitsCached = 0;

    return AP4_SUCCESS;
}

/*----------------------------------------------------------------------
|   AP4_BitReader::GetBitsRead
+---------------------------------------------------------------------*/
unsigned int
AP4_BitReader::GetBitsRead()
{
    return 8*m_Position - m_BitsCached;
}

/*----------------------------------------------------------------------
|   AP4_BitReader::ReadCache
+---------------------------------------------------------------------*/
AP4_BitReader::BitsWord
AP4_BitReader::ReadCache() const
{
    const AP4_UI08* out_ptr = m_Buffer.GetData()+m_Position;
    return (((AP4_BitReader::BitsWord) out_ptr[0]) << 24) |
           (((AP4_BitReader::BitsWord) out_ptr[1]) << 16) |
           (((AP4_BitReader::BitsWord) out_ptr[2]) <<  8) |
           (((AP4_BitReader::BitsWord) out_ptr[3])      );
}

/*----------------------------------------------------------------------
|   AP4_BitReader::ReadBits
+---------------------------------------------------------------------*/
AP4_UI32
AP4_BitReader::ReadBits(unsigned int n)
{
    if (n == 0) return 0;
    AP4_BitReader::BitsWord result;
    if (m_BitsCached >= n) {
        /* we have enough bits in the cache to satisfy the request */
        m_BitsCached -= n;
        result = (m_Cache >> m_BitsCached) & AP4_BIT_MASK(n);
    } else {
        /* not enough bits in the cache */
        AP4_BitReader::BitsWord word = ReadCache();
        m_Position += AP4_WORD_BYTES;

        /* combine the new word and the cache, and update the state */
        AP4_BitReader::BitsWord cache = m_Cache & AP4_BIT_MASK(m_BitsCached);
        n -= m_BitsCached;
        m_BitsCached = AP4_WORD_BITS - n;
        result = m_BitsCached ? (word >> m_BitsCached) | (cache << n) : word;
        m_Cache = word;
    }

    return result;
}

/*----------------------------------------------------------------------
|   AP4_BitReader::ReadBit
+---------------------------------------------------------------------*/
int
AP4_BitReader::ReadBit()
{
    AP4_BitReader::BitsWord result;
    if (m_BitsCached == 0) {
        /* the cache is empty */

        /* read the next word into the cache */
        m_Cache = ReadCache();
        m_Position += AP4_WORD_BYTES;
        m_BitsCached = AP4_WORD_BITS - 1;

        /* return the first bit */
        result = m_Cache >> (AP4_WORD_BITS - 1);
    } else {
        /* get the bit from the cache */
        result = (m_Cache >> (--m_BitsCached)) & 1;
    }
    return result;
}

/*----------------------------------------------------------------------
|   AP4_BitReader::PeekBits
+---------------------------------------------------------------------*/
AP4_UI32
AP4_BitReader::PeekBits(unsigned int n)
{
   /* we have enough bits in the cache to satisfy the request */
   if (m_BitsCached >= n) {
      return (m_Cache >> (m_BitsCached - n)) & AP4_BIT_MASK(n);
   } else {
      /* not enough bits in the cache, read the next word */
      AP4_BitReader::BitsWord word = ReadCache();

      /* combine the new word and the cache, and update the state */
      AP4_BitReader::BitsWord   cache = m_Cache & AP4_BIT_MASK(m_BitsCached);
      n -= m_BitsCached;
      return (word >> (AP4_WORD_BITS - n)) | (cache << n);
   }
}

/*----------------------------------------------------------------------
|   AP4_BitReader::PeekBit
+---------------------------------------------------------------------*/
int
AP4_BitReader::PeekBit()
{
   /* the cache is empty */
   if (m_BitsCached == 0) {
      /* read the next word into the cache */
      AP4_BitReader::BitsWord cache = ReadCache();

      /* return the first bit */
      return cache >> (AP4_WORD_BITS - 1);
   } else {
      /* get the bit from the cache */
      return (m_Cache >> (m_BitsCached-1)) & 1;
   }
}

/*----------------------------------------------------------------------
|   AP4_BitReader::SkipBits
+---------------------------------------------------------------------*/
void
AP4_BitReader::SkipBits(unsigned int n)
{
   if (n <= m_BitsCached) {
      m_BitsCached -= n;
   } else {
      n -= m_BitsCached;
      while (n >= AP4_WORD_BITS) {
         m_Position += AP4_WORD_BYTES;
         n -= AP4_WORD_BITS;
      }
      if (n) {
         m_Cache = ReadCache();
         m_BitsCached = AP4_WORD_BITS-n;
         m_Position += AP4_WORD_BYTES;
      } else {
         m_BitsCached = 0;
         m_Cache = 0;
      }
   }
}

/*----------------------------------------------------------------------
|   AP4_BitReader::SkipBit
+---------------------------------------------------------------------*/
void
AP4_BitReader::SkipBit()
{
   if (m_BitsCached == 0) {
      m_Cache = ReadCache();
      m_Position += AP4_WORD_BYTES;
      m_BitsCached = AP4_WORD_BITS - 1;
   } else {
      --m_BitsCached;
   }
}


