/*****************************************************************
|
|    AP4 - Strings
|
|    Copyright 2002-2006 Gilles Boccon-Gibod & Julien Boeuf
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
#include "Ap4String.h"
#include "Ap4Types.h"
#include "Ap4Utils.h"

/*----------------------------------------------------------------------
|   AP4_String::EmptyString
+---------------------------------------------------------------------*/
char AP4_String::EmptyString = 0;

/*----------------------------------------------------------------------
|   AP4_String::AP4_String
+---------------------------------------------------------------------*/
AP4_String::AP4_String() : m_Chars(&EmptyString), m_Length(0) {}

/*----------------------------------------------------------------------
|   AP4_String::AP4_String
+---------------------------------------------------------------------*/
AP4_String::AP4_String(const char* s) {
    if (s == NULL) {
        m_Chars = &EmptyString;
        m_Length = 0;
        return;
    }
    m_Length = AP4_StringLength(s);
    m_Chars = new char[m_Length+1];
    AP4_CopyMemory(m_Chars, s, m_Length+1);
}

/*----------------------------------------------------------------------
|   AP4_String::AP4_String
+---------------------------------------------------------------------*/
AP4_String::AP4_String(const char* s, AP4_Size size) :
    m_Chars(new char[size+1]), 
    m_Length(size)
{
    m_Chars[size] = 0;
    AP4_CopyMemory(m_Chars, s, size);
}

/*----------------------------------------------------------------------
|   AP4_String::AP4_String
+---------------------------------------------------------------------*/
AP4_String::AP4_String(const AP4_String& s) {
    m_Length = s.m_Length;
    m_Chars = new char[m_Length+1];
    AP4_CopyMemory(m_Chars, s.m_Chars, m_Length+1);
}

/*----------------------------------------------------------------------
|   AP4_String::AP4_String
+---------------------------------------------------------------------*/
AP4_String::AP4_String(AP4_Size size) {
    m_Length = size;
    m_Chars = new char[m_Length+1];
    for (unsigned int i=0; i<size+1; i++) m_Chars[i] = 0;
}

/*----------------------------------------------------------------------
|   AP4_String::~AP4_String
+---------------------------------------------------------------------*/
AP4_String::~AP4_String() 
{
    if (m_Chars != &EmptyString) delete[] m_Chars;
}

/*----------------------------------------------------------------------
|   AP4_String::operator=
+---------------------------------------------------------------------*/
const AP4_String&
AP4_String::operator=(const AP4_String& s)
{
    if (&s == this) return s;
    if (m_Chars != &EmptyString) delete[] m_Chars;
    m_Length = s.m_Length;
    m_Chars = new char[m_Length+1];
    AP4_CopyMemory(m_Chars, s.m_Chars, m_Length+1);

    return *this;
}
