/*****************************************************************
|
|    AP4 - Posix Random Byte Generator implementation
|
|    Copyright 2002-2015 Axiomatic Systems, LLC
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
#include <unistd.h>
#include <fcntl.h>

#include "Ap4Utils.h"

/*----------------------------------------------------------------------
|   AP4_System_GenerateRandomBytes
+---------------------------------------------------------------------*/
AP4_Result
AP4_System_GenerateRandomBytes(AP4_UI08* buffer, AP4_Size buffer_size)
{
    AP4_SetMemory(buffer, 0, buffer_size);
    int random = open("/dev/urandom", O_RDONLY);
    if (random < 0) {
        return AP4_FAILURE;
    }
    AP4_Result result = AP4_SUCCESS;
    while (buffer_size) {
        int nb_read = (int)read(random, buffer, buffer_size);
        if (nb_read <= 0) {
            result = AP4_ERROR_READ_FAILED;
            goto end;
        }
        if ((AP4_Size)nb_read > buffer_size) {
            result = AP4_ERROR_INTERNAL;
            goto end;
        }
        buffer_size -= nb_read;
        buffer += nb_read;
    }
    
end:
    close(random);
    return result;
}











