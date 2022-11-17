#!/usr/bin/env python3

import sys
import requests
import xml.etree.ElementTree as et

url = 'http://fantasticcontraption.com/retrieveLevel.php'

resp = requests.post(url, data = {'id': sys.argv[1], 'loadDesign': 1})
retrieve_level = et.fromstring(resp.text)
level = retrieve_level.find('level')
level_blocks = level.find('levelBlocks')
player_blocks = level.find('playerBlocks')

print(resp.text)
sys.exit(0)

def fcsim_type(tag, goal_block):
	if goal_block == 'true':
		if tag == 'NoSpinWheel':
			return 'FCSIM_GOAL_CIRCLE'
		else:
			return 'FCSIM_GOAL_RECT'
	d = { 'StaticRectangle'  : 'FCSIM_STAT_RECT',
	      'StaticCircle'     : 'FCSIM_STAT_CIRCLE',
	      'DynamicRectangle' : 'FCSIM_DYN_RECT', 
	      'DynamicCircle'    : 'FCSIM_DYN_CIRCLE',
	      'NoSpinWheel'      : 'FCSIM_WHEEL',
	      'ClockwiseWheel'   : 'FCSIM_CW_WHEEL',
	      'CounterClockwiseWheel' : 'FCSIM_CCW_WHEEL',
	      'SolidRod'         : 'FCSIM_SOLID_ROD',
	      'HollowRod'        : 'FCSIM_ROD' }
	return d[tag]

def handle_block(b):
	rotation = b.find('rotation').text
	position = b.find('position')
	x = position.find('x').text
	y = position.find('y').text
	width = b.find('width').text
	height = b.find('height').text
	goal_block = b.find('goalBlock').text
	joints = b.find('joints')
	j = []
	for jointed_to in joints.findall('jointedTo'):
		j.append(jointed_to.text)
	if len(j) > 0:
		j0 = j[0]
	else:
		j0 = '-1'
	if len(j) > 1:
		j1 = j[1]
	else:
		j1 = '-1'
	btype = fcsim_type(b.tag, goal_block)

	print('\t{', '{}, {}, {}, {}, {}, {},'.format(btype, x, y, width, height, rotation), '{', '{}, {}'.format(j0, j1), '} },')

print('struct fcsim_block blocks[] = {');
for b in player_blocks:
	handle_block(b)
for b in level_blocks:
	handle_block(b)
print('};')
