/*****************************************************************
|
|    Copyright 2002-2008 Axiomatic Systems, LLC
|
|    $Id: MetaData.java 196 2008-10-14 22:59:31Z bok $
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

package com.axiosys.bento4.metadata;

public class MetaData {
    public final static int TYPE_DATA = 0x64617461; // 'data' -> data
    public final static int TYPE_MEAN = 0x6d65616e; // 'mean' -> namespace
    public final static int TYPE_NAME = 0x6e616d65; // 'name' -> name
    public final static int TYPE_dddd = 0x2d2d2d2d; // '----' -> free form
    public final static int TYPE_cNAM = 0xa96e616d; // '©nam' -> name
    public final static int TYPE_cART = 0xa9415240; // '©ART' -> artist
    public final static int TYPE_cCOM = 0xa9636f6d; // '©com' -> composer
    public final static int TYPE_cWRT = 0xa9777274; // '©wrt' -> writer
    public final static int TYPE_cALB = 0xa9616c62; // '©alb' -> album
    public final static int TYPE_cGEN = 0xa967656e; // '©gen' -> genre
    public final static int TYPE_cGRP = 0xa9677270; // '©grp' -> group
    public final static int TYPE_cDAY = 0xa9646179; // '©day' -> date
    public final static int TYPE_cTOO = 0xa9746f6f; // '©too' -> tool
    public final static int TYPE_cCMT = 0xa9636d74; // '©cmt' -> comment
    public final static int TYPE_cLYR = 0xa96c7972; // '©lyr' -> lyrics
    public final static int TYPE_CPRT = 0x63707274; // 'cprt' -> copyright
    public final static int TYPE_TRKN = 0x74726b6e; // 'trkn' -> track#
    public final static int TYPE_DISK = 0x6469736b; // 'disk' -> disk#
    public final static int TYPE_COVR = 0x636f7672; // 'covr' -> cover art
    public final static int TYPE_DESC = 0x64657363; // 'desc' -> description
    public final static int TYPE_GNRE = 0x676e7265; // 'gnre' -> genre (ID3v1 index + 1)
    public final static int TYPE_CPIL = 0x6370696c; // 'cpil' -> compilation?
//    public final static int TYPE_TMPO = AP4_ATOM_TYPE('t','m','p','o'); // tempo
//    public final static int TYPE_RTNG = AP4_ATOM_TYPE('r','t','n','g'); // rating
//    public final static int TYPE_apID = AP4_ATOM_TYPE('a','p','I','D');
//    public final static int TYPE_cnID = AP4_ATOM_TYPE('c','n','I','D');
//    public final static int TYPE_atID = AP4_ATOM_TYPE('a','t','I','D');
//    public final static int TYPE_plID = AP4_ATOM_TYPE('p','l','I','D');
//    public final static int TYPE_geID = AP4_ATOM_TYPE('g','e','I','D');
//    public final static int TYPE_sfID = AP4_ATOM_TYPE('s','f','I','D');
//    public final static int TYPE_akID = AP4_ATOM_TYPE('a','k','I','D');
//    public final static int TYPE_aART = AP4_ATOM_TYPE('a','A','R','T');
//    public final static int TYPE_TVNN = AP4_ATOM_TYPE('t','v','n','n'); // TV network
//    public final static int TYPE_TVSH = AP4_ATOM_TYPE('t','v','s','h'); // TV show
//    public final static int TYPE_TVEN = AP4_ATOM_TYPE('t','v','e','n'); // TV episode name
//    public final static int TYPE_TVSN = AP4_ATOM_TYPE('t','v','s','n'); // TV show season #
//    public final static int TYPE_TVES = AP4_ATOM_TYPE('t','v','e','s'); // TV show episode #
//    public final static int TYPE_STIK = AP4_ATOM_TYPE('s','t','i','k');

    
    public MetaData() {
    }
}
