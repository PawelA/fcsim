#!/usr/bin/env python3

import requests
import argparse

url = 'http://fantasticcontraption.com/retrieveLevel.php'

parser = argparse.ArgumentParser(description='Get the XML of a design or level')
parser.add_argument('-l', type=int)
parser.add_argument('-d', type=int)
parser.add_argument('-o', type=str)

args = parser.parse_args()

if args.l:
	_id = args.l
	design = 0
elif args.d:
	_id = args.d
	design = 1
else:
	parser.error('-l or -d required')

resp = requests.post(url, data = {'id': _id, 'loadDesign': design})
print(resp.text)
