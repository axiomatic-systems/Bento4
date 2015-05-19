#! /usr/bin/env python

import sys
import json
import aes
import hashlib
from optparse import OptionParser

WV_DEFAULT_SERVER_URL  = 'https://license.uat.widevine.com/cenc/getcontentkey'
WV_DEFAULT_PROVIDER    = 'widevine_test'
WV_DEFAULT_SIGNING_KEY = '1ae8ccd0e7985cc0b6203a55855a1034afc252980e970ca90e5202689f947ab9'
WV_DEFAULT_SIGNING_IV  = 'd58ce954203b7c9a9a9d467f59839249'

try:
	import requests
except:
	raise Exception('"Requests" python module not installed. Please install it (use "pip install requests" or "easy_install requests" in a command shell, or consult the "Requests" documentation at http://docs.python-requests.org/en/latest/)')

def base64_encode(str):
	return str.encode('base64').replace('\n', '')


### Main
parser = OptionParser(usage="%prog [options]",
                         description="Request keys and PSSH data from a Widevine key server")
parser.add_option('-v', '--verbose', dest="verbose", action='store_true', default=False,
                      help="Be verbose")
parser.add_option('-d', '--debug', dest="debug", action='store_true', default=False,
                      help="Print debug information")
parser.add_option('-u', '--url', dest="server_url", default=WV_DEFAULT_SERVER_URL,
                  help="Widevine Server URL (default: "+WV_DEFAULT_SERVER_URL+")")
parser.add_option('-p', '--provider', dest="provider", default=WV_DEFAULT_PROVIDER,
                  help="Widevine provider/signer name (default: "+WV_DEFAULT_PROVIDER+")")
parser.add_option('-k', '--aes-signing-key', dest="aes_signing_key", default=WV_DEFAULT_SIGNING_KEY,
                  help="AES signing key")
parser.add_option('-i', '--aes-signing-iv', dest="aes_signing_iv", default=WV_DEFAULT_SIGNING_IV,
                  help="ES signing IV")
parser.add_option('-c', '--content-id', dest="content_id", default=None,
                  help="Content ID (hex)")
parser.add_option('-t', '--track-types', dest="track_types", default='SD,HD,AUDIO',
                  help="List of track types, separated by ','")
parser.add_option('-l', '--policy', dest="policy", default='',
                  help="Policy")

(options, args) = parser.parse_args()
if not options.content_id:
    print 'ERROR: missing --content-id option'
    parser.print_help()
    sys.exit(1)

rq_payload = {
	'content_id': base64_encode(options.content_id.decode('hex')),
	'drm_types':  ['WIDEVINE'],
	'policy':      options.policy
}

if options.track_types:
	rq_payload['tracks'] = [{'type': track_type} for track_type in options.track_types.split(',')]
else:
	rq_payload['tracks'] = []

rq_payload_json = json.dumps(rq_payload)
if options.debug:
	print 'Request Payload', rq_payload_json

sha1_hasher = hashlib.sha1()
sha1_hasher.update(rq_payload_json)
rq_payload_signature = aes.cbc_encrypt(sha1_hasher.digest(), options.aes_signing_key.decode('hex'), options.aes_signing_iv.decode('hex'))

post_body = {
	"request":base64_encode(rq_payload_json),
	"signature":base64_encode(rq_payload_signature),
	"signer": options.provider
}

if options.debug:
	print 'POST Body:'
	print post_body

post_url = options.server_url+'/'+options.provider
http_response = requests.post(post_url, data=json.dumps(post_body))

if options.debug:
	print 'Response:', http_response.text

parsed_response = json.loads(http_response.text)
parsed_payload = json.loads(parsed_response['response'].decode('base64'))

if options.debug:
	print 'JSON Payload:'
	print json.dumps(parsed_payload, indent=4)

if parsed_payload['status'] == 'OK':
	for track in parsed_payload['tracks']:
		print 'Track:'
		print '  Type =', track['type']
		print '  Key  =', track['key'].decode('base64').encode('hex')
		print '  KID  =', track['key_id'].decode('base64').encode('hex')
		print '  PSSH =', track['pssh'][0]['data'], '['+track['pssh'][0]['data'].decode('base64').encode('hex')+']'
else:
	print 'ERROR:', parsed_payload['status']
