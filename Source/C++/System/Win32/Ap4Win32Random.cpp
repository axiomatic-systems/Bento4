/*****************************************************************
|
|    AP4 - Win32 Random Byte Generator implementation
|
|    Copyright 2002-2011 Axiomatic Systems, LLC
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
#define _CRT_RAND_S
#include <stdlib.h>

#include "Ap4Utils.h"

/*----------------------------------------------------------------------
|   AP4_System_GenerateRandomBytes
+---------------------------------------------------------------------*/
AP4_Result
AP4_System_GenerateRandomBytes(AP4_UI08* buffer, AP4_Size buffer_size)
{
    AP4_SetMemory(buffer, 0, buffer_size);

	while (buffer_size) {
		unsigned int r;
		errno_t result = rand_s(&r);
		if (result != 0) return AP4_FAILURE;

		unsigned int chunk = buffer_size < sizeof(r) ? buffer_size : sizeof(r);
		memcpy(buffer, (void*)&r, chunk);
		buffer += chunk;
		buffer_size -= chunk;
	}

    return AP4_SUCCESS;
}











