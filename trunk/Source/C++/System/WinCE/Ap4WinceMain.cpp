/*****************************************************************
|
|    AP4 - Windows CE Utils
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
#include <windows.h>

/*----------------------------------------------------------------------
|   _tmain
+---------------------------------------------------------------------*/
extern int main(int argc, char** argv);

int
_tmain(int argc, wchar_t** argv, wchar_t** envp)
{
    char** argv_utf8 = new char*[argc*sizeof(char*)];
    int i;
    int result;

    /* allocate and convert args */
    for (i=0; i<argc; i++) {
        unsigned int arg_length = wcslen(argv[i]);
        argv_utf8[i] = new char[4*arg_length+1];
        WideCharToMultiByte(CP_UTF8, 0, argv[i], -1, argv_utf8[i], 4*arg_length+1, 0, 0);
    }

    result = main(argc, argv_utf8);


    /* cleanup */
    for (i=0; i<argc; i++) {
        delete [] argv_utf8[i];
    }
    delete[] argv_utf8;

    return result;
}
