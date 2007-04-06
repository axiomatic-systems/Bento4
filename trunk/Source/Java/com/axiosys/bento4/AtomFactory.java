package com.axiosys.bento4;

import java.io.IOException;
import java.io.RandomAccessFile;
import java.util.ArrayList;
import java.util.Iterator;
import java.util.List;

import com.axiosys.bento4.dcf.OdafAtom;
import com.axiosys.bento4.dcf.OhdrAtom;
import com.axiosys.bento4.ismacryp.IkmsAtom;
import com.axiosys.bento4.metadata.MetaDataTypeHandler;

public class AtomFactory {
    public static final AtomFactory DefaultFactory = new AtomFactory();
    
    private int context = 0;
    private List typeHandlers = new ArrayList();
    
    public AtomFactory() {
        // add builtin type handler
        typeHandlers.add(new MetaDataTypeHandler(this));
    }
    
    public Atom createAtom(RandomAccessFile file) throws IOException, InvalidFormatException {
        return createAtom(file, new int[] { (int) (file.length()-file.getFilePointer()) });
    }

    public void addTypeHandler(TypeHandler handler) {
        typeHandlers.add(handler);
    }
    
    public void setContext(int context) {
        this.context = context;
    }

    public int getContext() {
        return context;
    }

    Atom createAtom(RandomAccessFile source, int[] bytesAvailable /* by reference */) throws IOException, InvalidFormatException {
        Atom atom = null;
        
        // check that there are enough bytes for at least a header
        if (bytesAvailable[0] < Atom.HEADER_SIZE) return null;

        // remember current file offset
        long start = source.getFilePointer();

        // read atom size
        int size = source.readInt();

        if (size == 0) {
            // atom extends to end of file
            size = (int)(source.length()-start);
        }

        // check the size (we don't handle extended size yet)
        if (size > bytesAvailable[0]) {
            source.seek(start);
            return null;
        }

        if (size < 0) {
            // something is corrupted
            throw new InvalidFormatException("invalid atom size");
        }
        
        // read atom type
        int type = source.readInt();

        // create the atom
        switch (type) {
          case Atom.TYPE_STSD:
            atom = new StsdAtom(size, source, this);
            break;
                
          case Atom.TYPE_SCHM:
            atom = new SchmAtom(size, source);
            break;

          case Atom.TYPE_IKMS:
            atom = new IkmsAtom(size, source);
            break;

          case Atom.TYPE_TRAK:
            atom = new TrakAtom(size, source, this);
            break;
            
          case Atom.TYPE_TKHD:
            atom = new TkhdAtom(size, source);
            break;
            
          case Atom.TYPE_HDLR:
            atom = new HdlrAtom(size, source);
            break;
            
          case Atom.TYPE_ODAF:
              atom = new OdafAtom(size, source);
              break;
              
          case Atom.TYPE_OHDR:
              atom = new OhdrAtom(size, source, this);
              break;
            
          // container atoms
          case Atom.TYPE_MOOV:
          case Atom.TYPE_HNTI:
          case Atom.TYPE_STBL:
          case Atom.TYPE_MDIA:
          case Atom.TYPE_DINF:
          case Atom.TYPE_MINF:
          case Atom.TYPE_SCHI:
          case Atom.TYPE_SINF:
          case Atom.TYPE_UDTA:
          case Atom.TYPE_ILST:
          case Atom.TYPE_EDTS:
            atom = new ContainerAtom(type, size, false, source, this);
            break;

          // full container atoms
          case Atom.TYPE_META:
          case Atom.TYPE_ODKM:
            atom = new ContainerAtom(type, size, true, source, this);
            break;

          // sample entries
          case Atom.TYPE_MP4A:
            atom = new Mp4aSampleEntry(size, source, this);
            break;

          case Atom.TYPE_MP4V:
              atom = new Mp4vSampleEntry(size, source, this);
              break;

          case Atom.TYPE_ENCA:
              atom = new EncaSampleEntry(size, source, this);
              break;

          case Atom.TYPE_ENCV:
              atom = new EncvSampleEntry(size, source, this);
              break;
              
          default:
              // try all external type handlers
              for (Iterator i=typeHandlers.iterator(); i.hasNext();) {
                  TypeHandler handler = (TypeHandler)i.next();
                  atom = handler.createAtom(type, size, source, context);
                  if (atom != null) break;
              }
          
              // not type handler could handle this, create a generic atom
              if (atom == null) atom = new UnknownAtom(type, size, source);
              break;
        }

        // skip to the end of the atom
        bytesAvailable[0] -= size;
        source.seek(start+size);

        return atom;
    }
}
