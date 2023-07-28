#!/usr/local/bin/python3.9

import minimalmodbus
import serial
import struct

#substation address - minimalmodbus.Instrument
#Inital address, coil quantity, function code - intrument.read_registers(intial address, coil quantity, function code)

#CRC verification - automatic

def main():
	 

	# port name , sub-station address
	instrument = minimalmodbus.Instrument('/dev/cuaU0',1)
	instrument.debug = True
	instrument.serial.baudrate = 9600
	instrument.serial.timeout = 3

	instrument.serial.parity   = serial.PARITY_NONE




	temp = instrument.read_registers(1, 1, 4)
	print(temp)

	humid = instrument.read_registers(2, 1, 4)
	print(humid)

	both = instrument.read_registers(1, 2, 4)
	print(both)

	read_keep = instrument.read_registers(257, 1, 3)
	print(read_keep)
	


	
	instrument.serial.close()

main()

