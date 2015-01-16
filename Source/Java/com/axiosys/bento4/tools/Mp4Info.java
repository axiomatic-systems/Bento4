/*****************************************************************
|
|    Copyright 2002-2008 Axiomatic Systems, LLC
|
|    $Id: Mp4Info.java 196 2008-10-14 22:59:31Z bok $
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

package com.axiosys.bento4.tools;

import java.io.IOException;

import com.axiosys.bento4.File;
import com.axiosys.bento4.InvalidFormatException;
import com.axiosys.bento4.Track;

public class Mp4Info {

    /**
     * @param args
     * @throws InvalidFormatException 
     * @throws IOException 
     */
    public static void main(String[] args) throws IOException, InvalidFormatException {
        File file = new File(args[0]);
        Track[] tracks = file.getMovie().getTracks();
        for (int i=0; i<tracks.length; i++) {
            System.out.println("Track " + tracks[i].getId() + ":");
            System.out.println("  Type: " + tracks[i].getType());
        }
        System.out.println(file.getMovie().getMoovAtom());
    }
}