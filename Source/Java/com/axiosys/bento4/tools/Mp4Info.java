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
        System.out.println(file.getMovie().getAtom());
    }
}