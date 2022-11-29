#!/usr/bin/env python3

import sys
import requests
import xml.etree.ElementTree as et

url = 'http://fantasticcontraption.com/retrieveLevel.php'

resp = requests.post(url, data = {'id': sys.argv[1], 'loadDesign': 1})
print(resp.text)
