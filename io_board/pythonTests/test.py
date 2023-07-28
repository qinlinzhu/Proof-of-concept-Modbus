#!/usr/local/bin/python3.9
#  01 03 00 00 00 01 84 0A
# response : 01 03 02 00 00 B8 44

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




	temp = instrument.read_registers(0, 10, 3)
	print(temp)

	#value_two = instrument.read_bits(2, 10, 1)
	#print(value_two)

	#long_value = instrument.read_long(0000, 3, False)
	#print(long_value)

	#string_value = instrument.read_string(15, 16, 3)
	#print(string_value)
	


		
	

	
	
	instrument.serial.close()

main()

