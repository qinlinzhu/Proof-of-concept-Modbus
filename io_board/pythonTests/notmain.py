#!/usr/local/bin/python3.9
import serial
import time
import struct
#01 03 00 00 00 06 C5 C8


#setup
ser = serial.Serial(timeout=1)
ser.port = '/dev/cuaU0'
ser.baudrate = 9600
ser.bytesize = serial.EIGHTBITS
ser.parity=serial.PARITY_NONE
ser.stopbits = serial.STOPBITS_ONE



#connecting to serial
ser.open()
ser.flushInput()
ser.flushOutput()


# Demand from main station reading 1 holding resister from address 0000H
# '8B'	     : packet contains 8 bytes
# 0x01 	     : substation address
# 0x03       : function code
# 0x00, 0x00 : inital address
# 0x00, 0x01 : input quantity
# 0x84, 0x0A : CRC verification

req = struct.pack('8B', 0x01, 0x03, 0x00, 0x00, 0x00, 0x10, 0x44, 0x06)
ser.write(req)
print("message:")
print(req)

response = ser.read(20)
print("response:")
print(response)
