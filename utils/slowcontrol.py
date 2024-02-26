#!/usr/bin/env python3

import sys
import zmq
import json
from datetime import datetime
from time import sleep

import colors

host = "localhost"
if len(sys.argv) > 1:
    host =  sys.argv[1]

port = "22222"
if len(sys.argv) > 2:
    port =  sys.argv[2]

context = zmq.Context()
socket = context.socket(zmq.REQ)

url = f"tcp://{host}:{port}"

print(f"Connect to SlowControl socket... ({url})")
socket.connect(f"tcp://{host}:{port}")

jdict = {}
jdict['msg_type'] = 'Command'
jdict['msg_value'] = 'Status'
#jdict['msg_type'] = 'Config'

# jdoc is a str object
jdoc = json.JSONEncoder().encode(jdict)

for i in range(1,10):

    socket.send(jdoc.encode())
    msg = socket.recv()
    print(msg)

    sleep(1)
