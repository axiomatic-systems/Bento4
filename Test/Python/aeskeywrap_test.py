import skm
import random

key = '00112233445566778899AABBCCDDEEFF'
kek = '000102030405060708090A0B0C0D0E0F'
wk = skm.WrapKey(key, kek)

wk_ref = '1FA68B0A8112B447AEF34BD8FB5A7B829D3E862371D2CFE5'.decode('hex')
if wk != wk_ref:
	raise 'Boum!'

uk = skm.UnwrapKey(wk, kek)
if uk != key.decode('hex'):
	raise 'Boum!'

for i in range(1000):
	key = ''.join(chr(random.randint(0,255)) for _ in range(16))
	kek = ''.join(chr(random.randint(0,255)) for _ in range(16))
	wk = skm.WrapKey(key, kek)
	uk = skm.UnwrapKey(wk, kek)
	if uk != key:
		raise 'Bam!'