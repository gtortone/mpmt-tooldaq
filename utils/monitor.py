#!/usr/bin/env python3

import sys
import zmq
import json
from datetime import datetime

import colors

host = "localhost"
if len(sys.argv) > 1:
    host =  sys.argv[1]

port = "33333"
if len(sys.argv) > 2:
    port =  sys.argv[2]

context = zmq.Context()
socket = context.socket(zmq.SUB)

url = f"tcp://{host}:{port}"
print(f"Collecting updates from MPMT monitor... ({url})")
socket.connect (f"tcp://{host}:{port}")
socket.setsockopt_string(zmq.SUBSCRIBE, '')

while True:
    msg = socket.recv()
    # skip end char '\0'
    jobj = json.loads(msg[0:-1].decode('utf-8'))
    jstr = json.dumps(jobj, indent=4)
    print(colors.YELLOW, "MONITOR> ", datetime.now())
    print(colors.PLAIN, jstr, '\n')
