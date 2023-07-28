Modbus omron power supply test program README 


Purpose: To verify the configuration of modbus device and test if read commands produce expected responses and find unexpected behaviours.
	

HardWare: Omron S8VK-X48024A-EIP


Contact points: None encountered


Coding notes:
	batchmode program. 
	Controled from command line arguements. 
	Looks at Modbus TCP via ethernet

Structure of program: 

	Reads command line inputs
	sets up packet format 
	writes and sends packet 
	reads, processes and prints response


Observations during testing: 	- Tested 3 Omron power supplies all return back HLF true for "Years until the replacement reached HLF" in status data.
				- Few mistakes on Omron documentation.
				




References - https://www.simplymodbus.ca/TCP.htm
	   - https://www.tecnical.cat/PDF/OMRON/Power_supplies/T213-E1-01.pdf
	
Documents - file:///home/michael/Downloads/OmronPSU.pdf
