/*****************************************************************
|
|    AP4 - Main Header
|
|    Copyright 2002 Gilles Boccon-Gibod
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

#ifndef _AP4_H_
#define _AP4_H_

/*----------------------------------------------------------------------
|       includes
+---------------------------------------------------------------------*/
#include "Ap4Config.h"
#include "Ap4Types.h"
#include "Ap4Constants.h"
#include "Ap4Results.h"
#include "Ap4Debug.h"
#include "Ap4Utils.h"
#include "Ap4FileByteStream.h"
#include "Ap4Movie.h"
#include "Ap4Track.h"
#include "Ap4File.h"
#include "Ap4FileWriter.h"
#include "Ap4HintTrackReader.h"
#include "Ap4Processor.h"
#include "Ap4AtomFactory.h"
#include "Ap4SampleEntry.h"
#include "Ap4Sample.h"
#include "Ap4DataBuffer.h"
#include "Ap4SampleTable.h"
#include "Ap4SyntheticSampleTable.h"
#include "Ap4AtomSampleTable.h"
#include "Ap4UrlAtom.h"
#include "Ap4MoovAtom.h"
#include "Ap4MvhdAtom.h"
#include "Ap4TrakAtom.h"
#include "Ap4HdlrAtom.h"
#include "Ap4DrefAtom.h"
#include "Ap4TkhdAtom.h"
#include "Ap4MdhdAtom.h"
#include "Ap4StsdAtom.h"
#include "Ap4StscAtom.h"
#include "Ap4StcoAtom.h"
#include "Ap4StszAtom.h"
#include "Ap4EsdsAtom.h"
#include "Ap4SttsAtom.h"
#include "Ap4CttsAtom.h"
#include "Ap4StssAtom.h"
#include "Ap4FtypAtom.h"
#include "Ap4VmhdAtom.h"
#include "Ap4SmhdAtom.h"
#include "Ap4NmhdAtom.h"
#include "Ap4HmhdAtom.h"
#include "Ap4SchmAtom.h"
#include "Ap4FrmaAtom.h"
#include "Ap4TimsAtom.h"
#include "Ap4RtpAtom.h"
#include "Ap4SdpAtom.h"
#include "Ap4IkmsAtom.h"
#include "Ap4IsfmAtom.h"
#include "Ap4TrefTypeAtom.h"

#endif // _AP4_H_
