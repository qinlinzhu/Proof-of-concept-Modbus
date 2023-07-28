Modbus io board test program README 


Purpose: To verify the configuration of modbus device and test if read commands produce expected responses and find unexpected behaviours.
	
	
HardWare: Huaqingjun 32-Channel RS485+232+LAN Digital Input Module Data Acquisition 24VDC with Lights Modbus RTU/TCP for Switch 


Contact points: None encountered


Coding notes:
	batchmode program. 
	Controled from command line arguements. 
	Looks at 2 main areas - Modbus RTU via serial, Modbus TPC via ethernet

Structure of program: 

	Reads command line inputs
	sets up packet format 
	writes and sends packet 
	reads, processes and prints response


Observations during testing: 	serial test could only be ran once before no communications from device. 
				Fix by unplugging usb from computer and replugging.




References - https://www.simplymodbus.ca/TCP.htm
	   - https://fsqj81809248.en.made-in-china.com/product/xwkfpJFPJqhn/China-Huaqingjun-32-Channel-RS485-232-LAN-Digital-Input-Module-Data-Acquisition-24VDC-with-Lights-Modbus-RTU-TCP-for-Switch.html