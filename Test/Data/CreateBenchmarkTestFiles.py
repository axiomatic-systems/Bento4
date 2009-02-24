#! /usr/bin/env python

import sys
import os

filename_orig    = 'test-bench.mp4'
filename_dcf_cbc = 'test-bench.mp4.cbc.odf'
filename_dcf_ctr = 'test-bench.mp4.ctr.odf'
filename_pdcf_cbc = 'test-bench.cbc.pdcf.mp4'
filename_pdcf_ctr = 'test-bench.ctr.pdcf.mp4'

key1 = '000102030405060708090a0b0c0d0e0f'
iv1  = 'a0a1a2a3a4a5a6a7'

key2 = '000102030405060708090a0b0c0d0e0f'
iv2  = 'b0b1b2b3b4b5b6b7'

cmd = 'mp4encrypt --method OMA-PDCF-CTR --key 1:'+key1+':'+iv1+' --key 2:'+key2+':'+iv2+' '+filename_orig+' '+filename_pdcf_ctr
print 'Encrypting PDCF CTR file...'
print cmd
os.system(cmd)

cmd = 'mp4encrypt --method OMA-PDCF-CBC --key 1:'+key1+':'+iv1+' --key 2:'+key2+':'+iv2+' '+filename_orig+' '+filename_pdcf_cbc
print 'Encrypting PDCF CBC file...'
print cmd
os.system(cmd)

cmd = 'mp4dcfpackager --method CTR --key '+key1+':'+iv1+iv2+' '+filename_orig+' '+filename_dcf_ctr
print 'Encrypting DCF CTR file...'
print cmd
os.system(cmd)

cmd = 'mp4dcfpackager --method CBC --key '+key1+':'+iv1+iv2+' '+filename_orig+' '+filename_dcf_cbc
print 'Encrypting DCF CBC file...'
print cmd
os.system(cmd)
