/*****************************************************************
|
|    AP4 - File 
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

#ifndef _AP4_FILE_H_
#define _AP4_FILE_H_

/*----------------------------------------------------------------------
|   includes
+---------------------------------------------------------------------*/
#include "Ap4Types.h"
#include "Ap4List.h"
#include "Ap4AtomFactory.h"

/*----------------------------------------------------------------------
|   class references
+---------------------------------------------------------------------*/
class AP4_ByteStream;
class AP4_Movie;
class AP4_FtypAtom;

/*----------------------------------------------------------------------
|   file type/brands
+---------------------------------------------------------------------*/
const AP4_UI32 AP4_FILE_BRAND_QT__ = AP4_ATOM_TYPE('q','t',' ',' ');
const AP4_UI32 AP4_FILE_BRAND_ISOM = AP4_ATOM_TYPE('i','s','o','m');
const AP4_UI32 AP4_FILE_BRAND_MP41 = AP4_ATOM_TYPE('m','p','4','1');
const AP4_UI32 AP4_FILE_BRAND_MP42 = AP4_ATOM_TYPE('m','p','4','2');
const AP4_UI32 AP4_FILE_BRAND_3GP1 = AP4_ATOM_TYPE('3','g','p','1');
const AP4_UI32 AP4_FILE_BRAND_3GP2 = AP4_ATOM_TYPE('3','g','p','2');
const AP4_UI32 AP4_FILE_BRAND_3GP3 = AP4_ATOM_TYPE('3','g','p','3');
const AP4_UI32 AP4_FILE_BRAND_3GP4 = AP4_ATOM_TYPE('3','g','p','4');
const AP4_UI32 AP4_FILE_BRAND_3GP5 = AP4_ATOM_TYPE('3','g','p','5');
const AP4_UI32 AP4_FILE_BRAND_3G2A = AP4_ATOM_TYPE('3','g','2','a');
const AP4_UI32 AP4_FILE_BRAND_MMP4 = AP4_ATOM_TYPE('m','m','p','4');
const AP4_UI32 AP4_FILE_BRAND_M4A_ = AP4_ATOM_TYPE('M','4','A',' ');
const AP4_UI32 AP4_FILE_BRAND_M4P_ = AP4_ATOM_TYPE('M','4','P',' ');
const AP4_UI32 AP4_FILE_BRAND_MJP2 = AP4_ATOM_TYPE('m','j','p','2');

/*----------------------------------------------------------------------
|   AP4_File
+---------------------------------------------------------------------*/
class AP4_File {
public:
    // constructors and destructor
    AP4_File(AP4_Movie* movie);
    AP4_File(AP4_ByteStream& stream, 
             AP4_AtomFactory& atom_factory = AP4_DefaultAtomFactory::Instance);
    virtual ~AP4_File();

    // methods
    AP4_List<AP4_Atom>& GetOtherAtoms() { return m_OtherAtoms;}
    AP4_Movie*          GetMovie()      { return m_Movie;     }
    AP4_FtypAtom*       GetFileType()   { return m_FileType;  }
    AP4_Result          SetFileType(AP4_UI32     major_brand,
                                    AP4_UI32     minor_version,
                                    AP4_UI32*    compatible_brands = NULL,
                                    AP4_Cardinal compatible_brand_count = 0);
    virtual AP4_Result  Inspect(AP4_AtomInspector& inspector);

private:
    // members
    AP4_Movie*         m_Movie;
    AP4_FtypAtom*      m_FileType;
    AP4_List<AP4_Atom> m_OtherAtoms;
};

#endif // _AP4_FILE_H_
