import os
import sys
import os.path as path
from subprocess import check_output, CalledProcessError
import json
import io


KEYS = ['2CD7FA677504F13D5E58EF3495724231',
        '149B653DFB7E4E68D547851A36E17922',
        '6D1B508DC9F70DB07917E9C7598E2658',
        '67B9AC6201B3F27E711383AD8ABBF504']

IVS = ['59A8BA40D07FB49D312DA80A9E073CF4',
       'DEAAC9F680B88BE634B0CA6EFEC8C6C4',
       '198935C874A366AB5966197184C603C2',
       '939657EF6F82A4EF1C7BC378D0D294B6',
       '90C52422DBF731D337EC359EC8032FE6',
       '77B1F23CD0834AC6F21A9C77C55F86D6',
       '23DC84356625B11102A9CDBEB81E2CF5',
       '00000000000000000000000000000000',
       'FFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFF',
       'DE25ABCCB0B8BDD3', '00F31F8952D450AD',
       'F7BF85AE400E2524', 'CFA70DD931742898',
       'AD942C89D4D7FA81', '8253B2648FC2D8B1',
       '0000000000000000', 'FFFFFFFFFFFFFFFF']

METHODS_FOR_LINEAR_MP4     = ['OMA-PDCF-CBC', 'OMA-PDCF-CTR', 'OMA-DCF-CBC', 'OMA-DCF-CTR', 'ISMA-IAEC']
METHODS_FOR_FRAGMENTED_MP4 = ['MPEG-CENC', 'MPEG-CBC1', 'MPEG-CENS', 'MPEG-CBCS', 'PIFF-CBC', 'PIFF-CTR']

def Bento4Command(name, *args, **kwargs):
    executable = path.join(BIN_ROOT, name)
    cmd = [executable]
    arg_names = kwargs.keys()
    if 'method' in arg_names:
        arg_names.remove('method')
        arg_names = ['method']+arg_names
    for kwarg in arg_names:
        arg = kwarg.replace('_', '-')
        if isinstance(kwargs[kwarg], bool):
            cmd.append('--'+arg)
        else :
            if isinstance(kwargs[kwarg], list):
                for element in kwargs[kwarg]:
                    cmd.append('--'+arg)
                    cmd.append(element)
            else:
                cmd.append('--'+arg)
                cmd.append(kwargs[kwarg])

    cmd += args
    try:
        try:
            return check_output(cmd)
        except OSError as e:
            cmd[0] = path.basename(cmd[0])
            return check_output(cmd)
    except CalledProcessError as e:
        message = "binary tool failed with error %d" % e.returncode
        print cmd
        raise Exception(message)
    except OSError as e:
        raise Exception('executable "'+name+'" not found, ensure that it is in your path or in the directory '+options.exec_dir)


def Mp4Info(filename, *args, **kwargs):
    return Bento4Command('mp4info', filename, *args, **kwargs)

def DoDcfEncrypt(input, key, iv, method, output):
    lmethod = {'OMA-DCF-CBC':'CBC', 'OMA-DCF-CTR':'CTR'}[method]
    Bento4Command('mp4dcfpackager', input, output, method=lmethod, content_type='video/mp4', content_id='cid:bla@foo.bar', rights_issuer='http://bla.com/1234', key=key+':'+iv)

def DoDcfDecrypt(input, key, output):
    Bento4Command('mp4decrypt', input, output+'.tmp', key='1:'+key)
    Bento4Command('mp4extract', 'odrm/odda', output+'.tmp', output+'.odda', payload_only=True)
    f1 = open(output+'.odda', 'rb')
    f2 = open(output, 'wb+')
    f2.truncate()
    payload = f1.read()
    f2.write(payload[8:])
    f1.close()
    f2.close()
    os.unlink(output+'.tmp')
    os.unlink(output+'.odda')

def DoMp4Encrypt(input, key, iv, method, output):
    kwargs = {}
    if method == 'ISMA-IAEC':
        kwargs['kms_uri'] = 'http:foo.bar'

    Bento4Command('mp4encrypt', input, output, method=method, key=['1:'+key+':'+iv, '2:'+key+':'+iv], **kwargs)

def DoMp4Decrypt(input, key, output):
    Bento4Command('mp4decrypt', input, output, key=['1:'+key, '2:'+key])

def DoCompareMp4(file1, file2):
    retval = os.system(BIN_ROOT+'/CompareFilesTest '+file1+' '+file2)
    if retval != 0:
        raise Exception("Files " + file + " and " + file2 + " do not match")

def DoCompare(file1,  file2):
    f1 = open(file1, 'rb')
    f2 = open(file2, 'rb')

    d1 = f1.read()
    d2 = f2.read()

    f1.close()
    f2.close()

    if (d1 == d2):
        print "OK"
    else:
        raise("Files " + file + " and " + file2 + " do not match")

def DoCleanup(files):
    for file in files:
        os.unlink(file)

BIN_ROOT=sys.argv[1]
files = sys.argv[2:]
if len(sys.argv) >= 4:
    methods = sys.argv[3].split(',')
else:
    methods = None

counter = 0

for file in files:
    json_info = Mp4Info(file, format='json', fast=True)
    info = json.loads(json_info, strict=False)

    if methods is None:
        if info['movie']['fragments']:
            methods = METHODS_FOR_FRAGMENTED_MP4
        else:
            methods = METHODS_FOR_LINEAR_MP4

    for method in methods:
        for key in KEYS:
            for iv in IVS:
                counter += 1
                print 'Testing '+file+' with key='+key+", iv="+iv+", method="+method
                file_base = file+"."+method+"."+str(counter)
                enc_file = file_base+".enc.mp4"
                dec_file = file_base+".dec.mp4"
                if method.startswith('OMA-DCF'):
                    if len(iv) != 32: continue
                    DoDcfEncrypt(file, key, iv, method, enc_file)
                    DoDcfDecrypt(enc_file, key, dec_file)
                    DoCompare(file, dec_file)
                else:
                    DoMp4Encrypt(file, key, iv, method, enc_file)
                    DoMp4Decrypt(enc_file, key, dec_file)
                    DoCompareMp4(file, dec_file)
                DoCleanup([enc_file, dec_file])
