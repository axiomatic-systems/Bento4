/*****************************************************************
|
|    Copyright 2002-2008 Axiomatic Systems, LLC
|
|    $Id: Movie.java 196 2008-10-14 22:59:31Z bok $
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

package com.axiosys.bento4;

import java.util.ArrayList;

public class Movie {
    private ContainerAtom moov;
    private ArrayList     tracks = new ArrayList();
    
    public Movie(ContainerAtom moov) {
        this.moov = moov;
        
        for (int i=0; i<moov.getChildrenCount(); i++) {
            Atom child = moov.getChild(i);
            if (child.getType() == Atom.TYPE_TRAK) {
                TrakAtom trak = (TrakAtom)child;
                tracks.add(new Track(trak));
            }
        }
    }
    
    public Atom findAtom(String path) {
        return AtomUtils.findAtom(moov, path);
    }
    
    public Track[] getTracks() {
        Track[] result = new Track[tracks.size()];
        tracks.toArray(result);
        return result;
    }
    
    public int[] getTrackIds() {
        int[] result = new int[tracks.size()];
        for (int i=0; i<result.length; i++) {
            Track track = (Track)tracks.get(i);
            result[i] = track.getId();
        }
        
        return result;
    }
    
    public Track getTrackById(int id) {
        for (int i=0; i<tracks.size(); i++) {
            Track track = (Track)tracks.get(i);
            if (track.getId() == id) return track;
        }
        
        return null;
    }

    public int getTrackIndex(int id) {
        for (int i=0; i<tracks.size(); i++) {
            Track track = (Track)tracks.get(i);
            if (track.getId() == id) return i;
        }
        
        return -1;
    }
    
    public Atom getMoovAtom() {
        return moov;
    }
}
