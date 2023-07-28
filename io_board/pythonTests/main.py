#!/usr/local/bin/python3.9

import socket
import struct
import time

IP = '192.168.1.80'
PORT = 8080
BUFFER_SIZE = 96
sock = socket.socket(socket.AF_INET, socket.SOCK_STREAM)
sock.connect((IP, PORT))


# Demand from main station reading 1 holding resister from address 0000H
# '12B'      : packet contains 12 bytes
# 0x00, 0x01 : Transaction Identifier
# 0x00, 0x00 : Protocol Identifier
# 0x00, 0x06 : Message length (6 bytes to follow)
# 0x01 	     : substation address
# 0x03       : function code
# 0x00, 0x00 : inital address
# 0x00, 0x01 : input quantity

req = struct.pack('12B', 0x00, 0x01, 0x00, 0x00, 0x00, 0x06, 0x01, 0x03, 0x00, 0x04, 0x00, 0x2)
sock.send(req)
print("message:")
print(req)
response = sock.recv(1024)
print("response:")
print(response)




print('\nclosed socket')
sock.close()