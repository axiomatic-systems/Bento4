/*****************************************************************
|
|    Copyright 2002-2008 Axiomatic Systems, LLC
|
|    $Id: ContainerAtom.java 196 2008-10-14 22:59:31Z bok $
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

import java.io.DataOutputStream;
import java.io.IOException;
import java.io.RandomAccessFile;
import java.util.ArrayList;

public class ContainerAtom extends Atom implements AtomParent {
    protected final ArrayList children = new ArrayList();
    
    public ContainerAtom(int type, int size, boolean isFull, RandomAccessFile source) throws IOException {
        super(type, size, isFull, source);
    }

    public ContainerAtom(int type, int size, boolean isFull, RandomAccessFile source, AtomFactory atomFactory) throws IOException, InvalidFormatException {
        super(type, size, isFull, source);
        readChildren(atomFactory, source, getPayloadSize());
    }

    public Atom findAtom(String path) {
        return AtomUtils.findAtom(this, path);
    }
    
    protected void writeFields(DataOutputStream stream) throws IOException {
        writeChildren(stream);
    }
    
    protected void readChildren(AtomFactory atomFactory, RandomAccessFile source, int size) throws IOException, InvalidFormatException {
        int[] bytesAvailable = new int[] { size };
        Atom atom;
        
        // save and switch the context
        int savedContext = atomFactory.getContext();
        atomFactory.setContext(type);
        
        do {
            atom = atomFactory.createAtom(source, bytesAvailable);
            if (atom != null) children.add(atom);
        } while (atom != null);
        
        // restore the saved context
        atomFactory.setContext(savedContext);
    }

    protected void writeChildren(DataOutputStream stream) throws IOException {
        for (int i=0; i<children.size(); i++) {
            Atom atom = (Atom)children.get(i);
            atom.write(stream);
        }        
    }

    public int getChildrenCount() {
        return children.size();
    }
    
    public Atom getChild(int index) {
        return (Atom)children.get(index);
    }

    public Atom getChild(int type, int index) {
        return AtomUtils.findChild(children, type, index);
    }

    public String toString(String indentation) {
        StringBuffer result = new StringBuffer();
        result.append(super.toString(indentation));
        for (int i=0; i<children.size(); i++) {
            result.append("\n");
            result.append(((Atom)children.get(i)).toString(indentation+"  "));
        }
        
        return result.toString();  
    }
}
