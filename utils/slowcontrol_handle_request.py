#!/usr/bin/env python3

import sys
import zmq
import json
from datetime import datetime

import colors

host = "localhost"
if len(sys.argv) > 1:
    host =  sys.argv[2]

port = "22222"
if len(sys.argv) > 2:
    port =  sys.argv[3]

context = zmq.Context()
socket = context.socket(zmq.DEALER)
socket.setsockopt_string(zmq.IDENTITY, "MPMTSC1\0")

url = f"tcp://{host}:{port}"
print(f"Collecting requests from MPMT slow control... ({url})")
socket.connect(f"tcp://{host}:{port}")

msg = socket.recv_multipart()
print(msg)
# skip end char '\0'
jobj = json.loads(msg[1][0:-1].decode('utf-8'))
jstr = json.dumps(jobj, indent=4)
print(colors.YELLOW, "SlowControl Request> ", datetime.now())
print(colors.PLAIN, jstr, '\n')

# add blank
socket.send_string("", zmq.SNDMORE)

# add message payload
jdict = {}
jdict['msg_type'] = 'Config'
jdict['msg_value'] = 'blabla'

jdoc = json.JSONEncoder().encode(jdict)

socket.send(jdoc.encode())
