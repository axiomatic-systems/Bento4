import os
import sys


KEYS = ['2CD7FA677504F13D5E58EF3495724231', 
        '149B653DFB7E4E68D547851A36E17922',
        '6D1B508DC9F70DB07917E9C7598E2658',
        '67B9AC6201B3F27E711383AD8ABBF504']

IVS = ['59A8BA40D07FB49D', '312DA80A9E073CF4',
       'DEAAC9F680B88BE6', '34B0CA6EFEC8C6C4',
       '198935C874A366AB', '5966197184C603C2',
       '939657EF6F82A4EF', '1C7BC378D0D294B6',
       '90C52422DBF731D3', '37EC359EC8032FE6',
       '77B1F23CD0834AC6', 'F21A9C77C55F86D6',
       '23DC84356625B111', '02A9CDBEB81E2CF5',
       'DE25ABCCB0B8BDD3', '00F31F8952D450AD',
       'F7BF85AE400E2524', 'CFA70DD931742898',
       'AD942C89D4D7FA81', '8253B2648FC2D8B1',
       '71B2678CF765EF99', '3B52F5E2EFCB63A1',
       '428C7972208A0EDC', '4C76C3C3CAE92158',
       '79B10C489AC962CF', '4D36B77BFAEA63E3',
       '0D6586089F9E49E5', '63BF7AEAD8393715',
       '7B9A02EE1954EA91', '4661559786ED2AEF',
       'F43A530B9820CFA8', '7DD52B5D1F784BC7',
       '0000000000000000', 'FFFFFFFFFFFFFFFF']

METHODS = ['OMA-PDCF-CBC', 'OMA-PDCF-CTR', 'OMA-DCF-CBC', 'OMA-DCF-CTR', 'ISMA-IAEC']

def DoDcfEncrypt(input, key, iv, method, output):
    lmethod = {'OMA-DCF-CBC':'CBC', 'OMA-DCF-CTR':'CTR'}[method]
    cmd = BIN_ROOT+'/mp4dcfpackager --method '+lmethod+' --content-type video/mp4 --content-id cid:bla@foo.bar --rights-issuer http://bla.com/1234 --key '+key+':'+iv+iv+' '+input+' '+output
    print cmd
    os.system(cmd)
    
def DoDcfDecrypt(input, key, output):
    cmd = BIN_ROOT+"/mp4decrypt --key 1:"+key+' '+input+' '+output+'.tmp'
    print cmd
    os.system(cmd)
    cmd = BIN_ROOT+'/mp4extract --payload-only odrm/odda '+output+'.tmp '+output+'.odda'
    print cmd
    os.system(cmd)
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
    if method == 'ISMA-IAEC':
        options = ' --kms-uri http:foo.bar '
    else:
        options = ''
        
    cmd = BIN_ROOT+"/mp4encrypt --method "+method+options+" --key 1:"+key+":"+iv+" --key 2:"+key+":"+iv+' "'+input+'" "'+output+'"'
    print cmd
    os.system(cmd)

def DoMp4Decrypt(input, key, output):
    cmd = BIN_ROOT+"/mp4decrypt --key 1:"+key+" --key 2:"+key+' "'+input+'" "'+output+'"'
    print cmd
    os.system(cmd)

def DoCompareMp4(file1, file2):
    retval = os.system(BIN_ROOT+'/CompareFilesTest '+file1+' '+file2)
    if retval != 0:
        raise("Files " + file + " and " + file2 + " do not match")
    
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
        
BIN_ROOT = sys.argv[1]
files = sys.argv[2:]
counter = 0
for key in KEYS:
    for iv in IVS:
        counter += 1
        for method in METHODS:
            for file in files:
                print 'Testing '+file+' with key='+key+", iv="+iv+", method="+method
                file_base = file+"."+method+"."+str(counter) 
                enc_file = file_base+".enc.mp4"
                dec_file = file_base+".dec.mp4"
                if method.startswith('OMA-DCF'):
                    DoDcfEncrypt(file, key, iv, method, enc_file)
                    DoDcfDecrypt(enc_file, key, dec_file)
                    DoCompare(file, dec_file)
                else:
                    DoMp4Encrypt(file, key, iv, method, enc_file)
                    DoMp4Decrypt(enc_file, key, dec_file)
                    DoCompareMp4(file, dec_file)
                DoCleanup([enc_file, dec_file])
