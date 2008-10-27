
SUCCESS                               =  0
FAILURE                               = -1
ERROR_OUT_OF_MEMORY                   = -2
ERROR_INVALID_PARAMETERS              = -3
ERROR_NO_SUCH_FILE                    = -4
ERROR_PERMISSION_DENIED               = -5
ERROR_CANNOT_OPEN_FILE                = -6
ERROR_EOS                             = -7
ERROR_WRITE_FAILED                    = -8
ERROR_READ_FAILED                     = -9
ERROR_INVALID_FORMAT                  = -10
ERROR_NO_SUCH_ITEM                    = -11
ERROR_OUT_OF_RANGE                    = -12
ERROR_INTERNAL                        = -13
ERROR_INVALID_STATE                   = -14
ERROR_LIST_EMPTY                      = -15
ERROR_LIST_OPERATION_ABORTED          = -16
ERROR_INVALID_RTP_CONSTRUCTOR_TYPE    = -17
ERROR_NOT_SUPPORTED                   = -18
ERROR_INVALID_TRACK_TYPE              = -19
ERROR_INVALID_RTP_PACKET_EXTRA_DATA   = -20
ERROR_BUFFER_TOO_SMALL                = -21
ERROR_NOT_ENOUGH_DATA                 = -22

RESULT_EXCEPTION_MAP = {
    FAILURE:                             (Exception, ''),
    ERROR_OUT_OF_MEMORY:                 (MemoryError, ''),
    ERROR_INVALID_PARAMETERS:            (ValueError, 'Invalid parameter '),
    ERROR_NO_SUCH_FILE:                  (IOError, 'No such file '),
    ERROR_PERMISSION_DENIED:             (IOError, 'Permission denied '),
    ERROR_CANNOT_OPEN_FILE:              (IOError, 'Cannot open file '),
    ERROR_EOS:                           (EOFError, ''),
    ERROR_WRITE_FAILED:                  (IOError, 'Write failed '),
    ERROR_READ_FAILED:                   (IOError, 'Read failed '),
    ERROR_INVALID_FORMAT:                (ValueError, 'Invalid format '),
    ERROR_NO_SUCH_ITEM:                  (LookupError, ''),
    ERROR_OUT_OF_RANGE:                  (IndexError, ''),
    ERROR_INTERNAL:                      (RuntimeError, 'Bento4 internal error '),
    ERROR_INVALID_STATE:                 (RuntimeError, 'Bento4 invalid state'),
    ERROR_LIST_EMPTY:                    (IndexError, 'List empty '),
    ERROR_LIST_OPERATION_ABORTED:        (RuntimeError, 'List operation aborted '),
    ERROR_INVALID_RTP_CONSTRUCTOR_TYPE:  (ValueError, 'Invalid RTP constructor type '),
    ERROR_NOT_SUPPORTED:                 (NotImplementedError, ''),
    ERROR_INVALID_TRACK_TYPE:            (ValueError, 'Invalid track type '),
    ERROR_INVALID_RTP_PACKET_EXTRA_DATA: (ValueError, 'Invalid Rtp packet extra data '),
    ERROR_BUFFER_TOO_SMALL:              (MemoryError, 'Buffer too small '),
    ERROR_NOT_ENOUGH_DATA:               (IOError, 'Not enough data ')
}

def check_result(result, msg=''):
    # shortcut
    if result == SUCCESS:
        return
    try:
        exception, msg_prefix = RESULT_EXCEPTION_MAP[result]
    except KeyError:
        raise RuntimeError("Bento4 unknown error: code %d" % result)
    raise exception(msg_prefix+msg)
    
    
