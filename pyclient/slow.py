#!/usr/bin/python3

import zmq
import json

ctx = zmq.Context()
sock = ctx.socket(zmq.REQ)
sock.connect("tcp://10.1.1.34:60000")
msgid = 1

msg = {}
msg["uuid"] = "4ddc58d9-bc6c-4ab9-b20f-c4e58895db78"
msg["msg_id"] = msgid
msg["msg_time"] = "2024-11-08T16:56:44.616165Z"
msg["msg_type"] = "Command"
msg["msg_value"] = "hv-get-status"
msg["var1"] = "6"
#print(json.dumps(msg))

sock.send_string(json.dumps(msg) + '\0')
res = sock.recv()
print(res)
msgid = msgid + 1

# =======================================

msg.clear()
msg["uuid"] = "4ddc58d9-bc6c-4ab9-b20f-c4e58895db78"
msg["msg_id"] = msgid
msg["msg_time"] = "2024-11-08T16:57:44.616165Z"
msg["msg_type"] = "Command"
msg["msg_value"] = "rc-read-register"
msg["var1"] = "3"
#print(json.dumps(msg))

sock.send_string(json.dumps(msg) + '\0')
res = sock.recv()
print(res)
msgid = msgid + 1

