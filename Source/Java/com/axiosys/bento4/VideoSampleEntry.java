/*****************************************************************
|
|    Copyright 2002-2008 Axiomatic Systems, LLC
|
|    $Id: VideoSampleEntry.java 196 2008-10-14 22:59:31Z bok $
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

import java.io.IOException;
import java.io.RandomAccessFile;

public class VideoSampleEntry extends MpegSampleEntry {
    private int    width;           
    private int    height;
    private int    horizontalResolution;
    private int    verticalResolution;
    private int    frameCount;
    private int    depth;
    private String compressorName;
    
    protected VideoSampleEntry(int format, int size, RandomAccessFile source, AtomFactory atomFactory) throws IOException, InvalidFormatException {
        super(format, size, source, atomFactory);
    }

    int getWidth()                { return width;                }
    int getHeight()               { return height;               }
    int getHorizontalResolution() { return horizontalResolution; }
    int getVerticalResolution()   { return verticalResolution;   }
    int getFrameCount()           { return frameCount;           }
    int getDepth()                { return depth;                }
    String getCompressorName()    { return compressorName;       }
    
    protected void readFields(RandomAccessFile source) throws IOException {
        super.readFields(source);

        source.skipBytes(16);
        width = source.readUnsignedShort();
        height = source.readUnsignedShort();
        horizontalResolution = source.readInt();
        verticalResolution = source.readInt();
        source.skipBytes(4);
        frameCount = source.readUnsignedShort();

        byte[] str = new byte[32];
        source.readFully(str);
        int str_size = 0;
        while (str[str_size] != 0) str_size++;
        compressorName = new String(str, 0, str_size);

        depth = source.readUnsignedShort();
        source.skipBytes(2);
    }
    
    public String toString(String indentation) {
        StringBuffer result = new StringBuffer();
        result.append(indentation+"[" + typeString(type) + "] size=" + getHeaderSize() + "+" + getPayloadSize());
        result.append("\n" + indentation + "  width                 = " + width);
        result.append("\n" + indentation + "  height                = " + height);
        result.append("\n" + indentation + "  horizontal_resolution = " + horizontalResolution);
        result.append("\n" + indentation + "  vertical_resolution   = " + verticalResolution);
        result.append("\n" + indentation + "  frame_count           = " + frameCount);
        result.append("\n" + indentation + "  depth                 = " + depth);
        result.append("\n" + indentation + "  compressor_name       = " + compressorName);
        for (int i=0; i<children.size(); i++) {
            result.append("\n");
            result.append(((Atom)children.get(i)).toString(indentation+"  "));
        }
        
        return result.toString();  
    }
    
    public String toString() {
        return toString("");
    }
}
