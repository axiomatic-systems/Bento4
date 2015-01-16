/*****************************************************************
|
|    Copyright 2002-2008 Axiomatic Systems, LLC
|
|    $Id: Track.java 196 2008-10-14 22:59:31Z bok $
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

public class Track {

    public static final Type TYPE_UNKNOWN = new Type("Unknown");
    public static final Type TYPE_AUDIO   = new Type("Audio");
    public static final Type TYPE_VIDEO   = new Type("Video");
    public static final Type TYPE_HINT    = new Type("Hint");
    public static final Type TYPE_SYSTEMS = new Type("Systems");

    public static class Type {
        public static final int SOUN = 0x736f756e;
        public static final int VIDE = 0x76696465;
        public static final int HINT = 0x68696e74;
        public static final int SDSM = 0x7364736d;
        public static final int ODSM = 0x6f64736d;
        
        private String description;
        
        public static Type findType(int handlerType) {
            switch (handlerType) {
                case SOUN: return TYPE_AUDIO;
                case VIDE: return TYPE_VIDEO;
                case HINT: return TYPE_HINT;
                case SDSM:
                case ODSM: return TYPE_SYSTEMS;
                default:   return TYPE_UNKNOWN;
            }
        }
        
        public Type(String description) {
            this.description = description;
        }
        
        public String toString() {
            return description;
        }
    }
        
    private Type     type;
    private TrakAtom trakAtom;
    
    public Track(TrakAtom trak) {
        trakAtom = trak;
        HdlrAtom hdlr = (HdlrAtom)trak.findAtom("mdia/hdlr");
        if (hdlr != null) {
            type = Type.findType(hdlr.getHandlerType());
        } else {
            type = TYPE_UNKNOWN;
        }
    }
    
    public Type getType() {
        return type;
    }
    
    public int getId() {
        return trakAtom.getId();
    }
    
    public TrakAtom getTrakAtom() {
        return trakAtom;
    }
}
