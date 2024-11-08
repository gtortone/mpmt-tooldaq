#!/usr/bin/python3

import zmq
import json

ctx = zmq.Context()
sock = ctx.socket(zmq.REQ)
sock.connect("tcp://10.1.1.34:60000")

msg = {}
msg["uuid"] = 12345
msg["msg_id"] = 12
msg["msg_time"] = 0
msg["msg_type"] = "Command"
#msg["msg_value"] = "Status"
msg["msg_value"] = "hv-get-status"
#msg["msg_value"] = "hv-set-rampup-rate"
msg["var1"] = "3"

sock.send_string(json.dumps(msg))
res = sock.recv()
print(res)

sock.close()
