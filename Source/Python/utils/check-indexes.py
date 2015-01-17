#! /usr/bin/env python

import mp4utils
import sys
import os.path as path

SCRIPT_PATH = path.abspath(path.dirname(__file__))
sys.path += [SCRIPT_PATH]

platform = sys.platform
if platform.startswith('linux'):
    platform = 'linux-x86'
elif platform.startswith('darwin'):
    platform = 'macosx'

class Options(object): pass

options = Options()
options.debug = False
options.verbose = False
options.min_buffer_time = 0.0
options.exec_dir = path.join(SCRIPT_PATH, 'bin', platform)

class Sidx(object): pass

mp4_file = mp4utils.Mp4File(options, mp4utils.MediaSource(sys.argv[1]))
moof_offsets = []
sidx_anchors = []
for atom in mp4_file.atoms:
	if atom.type == 'moof':
		moof_offsets.append(atom.position)
	elif atom.type == 'sidx':
		sidx_anchors.append(atom.position+atom.size)

print 'sidx anchors:', sidx_anchors
print len(moof_offsets), 'moof boxes found:', moof_offsets

sidx = None
sidx_index = 0
for atom in mp4_file.tree:
	if atom['name'] == 'sidx':
		sidx = Sidx()
		sidx.entries = []
		sidx.reference_id = int(atom['reference_ID'])
		sidx.timescale = int(atom['timescale'])
		sidx.earliest_presentation_time = int(atom['earliest_presentation_time'])
		sidx.first_offset = int(atom['first_offset'])
		for (name, value) in atom.items():
			if name.startswith('entry '):
				entry = {}
				for field in value.split(', '):
					(field_name, field_value) = field.split('=')
					entry[field_name] = int(field_value)
				sidx.entries.append(entry)

		count = 0
		offset = sidx.first_offset
		for entry in sidx.entries:
			#print entry
			if entry['reference_type'] == 0:
				#print sidx_anchors[sidx_index]+offset
				if sidx_anchors[sidx_index]+offset not in moof_offsets:
					print 'sidx offset', sidx_anchors[sidx_index], '+', offset, 'does not point to a moof box'
			offset += entry['referenced_size']
		sidx_index += 1
		print 'sidx OK'
	elif atom['name'] == 'mfra':
		for child in atom['children']:
			if child['name'] == 'tfra':
				for (name, value) in child.items():
					if name.startswith('entry'):
						entry = {}
						for field in value.split(', '):
							(field_name, field_value) = field.split('=')
							entry[field_name] = int(field_value)
						if entry['moof_offset'] not in moof_offsets:
							print 'tfra offset', moof_offset, 'does not point to a moof box'
		print 'mfra OK'

print '=== Done ==='