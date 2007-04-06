package com.axiosys.bento4.dcf;

public class OmaDcf {
    // encryption
    public static final int ENCRYPTION_METHOD_NULL    = 0;
    public static final int ENCRYPTION_METHOD_AES_CBC = 1;
    public static final int ENCRYPTION_METHOD_AES_CTR = 2;
    
    // padding
    public static final int PADDING_SCHEME_NONE       = 0;
    public static final int PADDING_SCHEME_RFC_2630   = 1;
    
    public static String encryptionMethodToString(int method) {
        switch (method) {
        case ENCRYPTION_METHOD_NULL:
            return "Null";
        case ENCRYPTION_METHOD_AES_CBC:
            return "AES CBC";
        case ENCRYPTION_METHOD_AES_CTR:
            return "AES CTR";
        default:
            return "Unknown";
        }
    }
    
    public static String paddingSchemeToString(int scheme) {
        switch (scheme) {
        case PADDING_SCHEME_NONE:
            return "None";
        case PADDING_SCHEME_RFC_2630:
            return "RFC 2630";
        default:
            return "Unknown";
        }
    }
    

}
