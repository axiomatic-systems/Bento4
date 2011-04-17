/*****************************************************************
|
|    AP4 - Main Header
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
#include "Ap4.h"

/*----------------------------------------------------------------------
|   AP4::AP4
+---------------------------------------------------------------------*/
// this constructor can be used to detect if the platform's loader
// correctly constructs static C++ objects
static AP4 AP4_LoaderCheck;
AP4::AP4() : m_ConstructedByLoader(true)
{
}

/*----------------------------------------------------------------------
|   AP4::Initialize
+---------------------------------------------------------------------*/
AP4_Result
AP4::Initialize()
{
    AP4_Result result = AP4_SUCCESS;
#if defined(AP4_CONFIG_CONSTRUCT_STATICS_ON_INITIALIZE)
    if (!AP4_DefaultAtomFactory::Instance.m_Initialized) {
        result = AP4_DefaultAtomFactory::Instance.Initialize();
        if (AP4_FAILED(result)) return result;
    }
    if (!AP4_DefaultBlockCipherFactory::Instance.m_Initialized) {
        result = AP4_DefaultBlockCipherFactory::Instance.Initialize();
        if (AP4_FAILED(result)) return result;
    }
    if (!AP4_MetaData::Initialized()) {
        AP4_MetaData::Initialize();
    }
#endif
    
    return result;
}

/*----------------------------------------------------------------------
|   AP4_Terminate
+---------------------------------------------------------------------*/
AP4_Result
AP4::Terminate()
{
#if defined(AP4_CONFIG_DESTRUCT_STATICS_ON_TERMINATE)
    if (!AP4_LoaderCheck.m_ConstructedByLoader) {
        if (AP4_DefaultAtomFactory::Instance.m_Initialized) {
            AP4_DefaultAtomFactory::Instance.~AP4_DefaultAtomFactory();
        }
        if (AP4_DefaultBlockCipherFactory::Instance.m_Initialized) {
            AP4_DefaultBlockCipherFactory::Instance.~AP4_DefaultBlockCipherFactory();
        }
        if (AP4_MetaData::Initialized()) {
            AP4_MetaData::UnInitialize();
        }
    }
#endif

    return AP4_SUCCESS;
} 
