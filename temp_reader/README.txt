Modbus temperature reader test program README 


Purpose: To verify the configuration of modbus device and test if read commands produce expected responses and find unexpected behaviours.
	

HardWare: XY-MD02


Contact points: None encountered


Coding notes:
	batchmode program. 
	Controled from command line arguements. 
	Looks at Modbus RTU via RS485 serial

Structure of program: 

	Reads command line inputs
	sets up packet format 
	writes and sends packet 
	reads, processes and prints response


Observations during testing: 	serial test could only be ran once before no communications from device. 
				Fix by unplugging usb from computer and replugging.
				temperature and humidity might not be 100% accurate




References - https://www.simplymodbus.ca/TCP.htm
	   - http://www.sah.rs/media/sah/techdocs/xy-md02-manual.pdf

Documents - file:///home/michael/Downloads/xy-md02-manual.pdf
