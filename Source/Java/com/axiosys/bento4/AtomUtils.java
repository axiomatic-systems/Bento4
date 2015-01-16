/*****************************************************************
|
|    Copyright 2002-2008 Axiomatic Systems, LLC
|
|    $Id: AtomUtils.java 196 2008-10-14 22:59:31Z bok $
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

import java.util.Iterator;
import java.util.List;

public class AtomUtils {
    public static Atom findAtom(AtomParent parent, String path) {
        Atom atom = null;
        while (path != null) {
            int separator = path.indexOf('/');
            String atomName;
            int index = 0;
            if (separator > 0) {
                atomName = path.substring(0, separator);
                path = path.substring(separator+1);
            } else {
                atomName = path;
                path = null;
            }
            
            if (atomName.length() != 4) {
                // we need at least 3 more chars
                if (atomName.length() < 7) return null;
                
                // parse the name trailer
                if (atomName.charAt(4) != '[' || atomName.charAt(atomName.length()-1) != ']') {
                    return null;
                }
                String indexString = atomName.substring(5, atomName.length()-1);
                index = Integer.parseInt(indexString);
            }
            
            int type = Atom.nameToType(atomName);
            atom = parent.getChild(type, index);
            if (path == null) return atom;
            if (atom instanceof AtomParent) {
                parent = (AtomParent)atom;
            } else {
                return null;
            }
        }
        
        return atom;
    }

    public static Atom findChild(List atoms, int type, int index) {
        for (Iterator i = atoms.iterator(); i.hasNext();) {
            Atom atom = (Atom)i.next();
            if (atom.getType() == type) {
                if (index-- == 0) return atom;
            }
        }
        
        return null;
    }
}
