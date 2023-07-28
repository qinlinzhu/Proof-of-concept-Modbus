#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <errno.h>
#include <termios.h>
#include <fcntl.h> 



int temp();
int humid();
int continuous();
int ioSerial();
unsigned char process(  const unsigned char *array, const unsigned char *buf );
int Usage( const int code );
static uint16_t MODBUS_CRC16( const unsigned char *buf, unsigned int len );
void printMessage( const unsigned char *array );



int main(int argc, char **argv) 
{
	extern char *optarg;
	extern int opterr;
	extern int optind;
	extern int optopt;
	extern int optreset;
	
	if ( argc < 2 )
		Usage( 0 );

	int c, code = 0;
	
	while ((c = getopt(argc, argv, "thcH")) != -1) 
	{
		
		switch (c) 
		{
			case 'H':
				code = Usage( 0 );
				break;

			case 't':
				code = temp();
				break;

			case 'h':
				code = humid();
				break;

			case 'c':
				code = continuous();
				break;

			default:
				code = Usage( 1 );
				break;
		}
	}
	return code;
}



/*
 *  Function:
 *	temp() - creates data array
 *
 *  Synopsis:
 *	creates temperature command frame and sends it to serial
 */

int temp()
{
	int code = 0;
	unsigned char array[8] = {0x01, 0x04, 0x00, 0x01, 0x00, 0x01};
	code = ioSerial(array);
	return code;
}



/*
 *  Function:
 *	humid() - creates data array
 *
 *  Synopsis:
 *	creates humidity command frame and sends it to serial
 */

int humid()
{
	int code = 0;
	unsigned char array[8] = {0x01, 0x04, 0x00, 0x02, 0x00, 0x01};
	code = ioSerial(array);
	return code;
}



/*
 *  Function:
 *	continuous() - creates data array
 *
 *  Synopsis:
 *	creates continuous read temperature and humidity command frame and sends it to serial
 */

int continuous()
{
	int code = 0;
	unsigned char array[8] = {0x01, 0x04, 0x00, 0x01, 0x00, 0x02};
	code = ioSerial(array);
	return code;
}



/*
 *  Function:
 *	ioSerial() - process serial RS485 modbus
 *
 *  Synopsis:
 *	Creates modbus data, writes data to PLC, reads response from PLC
 */

int ioSerial( unsigned char *array )
{
	int code = 0;
	uint16_t crc =  MODBUS_CRC16(array, 6);
	array[6] = (crc >> 0) & 0xFF;
	array[7] = (crc >> 8) & 0xFF;
	
	printMessage( array );

	int serial_port = open("/dev/cuaU0", O_RDWR);
	struct termios tty;

	if(tcgetattr(serial_port, &tty) != 0) 
	{
		printf("Error %i from tcgetattr: %s\n", errno, strerror(errno));
	}
	
	tty.c_cflag &= ~PARENB;			// Clear parity bit, disabling parity
	tty.c_cflag &= ~CSTOPB;			// Clear stop field, only one stop bit used
	tty.c_cflag |= CS8;			// 8 bits per byte
	tty.c_cflag &= ~CRTSCTS;		// Disable RTS/CTS hardware flow control 
	tty.c_cflag |= CREAD | CLOCAL;		// Turn on READ & ignore ctrl lines 
	tty.c_lflag &= ~ICANON;			// Disable canonical mode
	tty.c_lflag &= ~ECHO;			// Disable echo
	tty.c_lflag &= ~ECHOE;			// Disable erasure
	tty.c_lflag &= ~ECHONL;			// Disable new-line echo
	tty.c_lflag &= ~ISIG;			// Disable signal chars
	
	tty.c_iflag &= ~(IXON | IXOFF | IXANY);					// Turn off s/w flow ctrl
	tty.c_iflag &= ~(IGNBRK|BRKINT|PARMRK|ISTRIP|INLCR|IGNCR|ICRNL);	// Disable any special handling of received bytes

	tty.c_oflag &= ~OPOST;			// Prevent special interpretation of output bytes (e.g. newline chars)
	tty.c_oflag &= ~ONLCR;			// Prevent conversion of newline to carriage return/line feed

	tty.c_cc[VTIME] = 10;    
	tty.c_cc[VMIN] = 0;

	// change braud rate here
	cfsetispeed(&tty, B9600);
	cfsetospeed(&tty, B9600);

	if (tcsetattr(serial_port, TCSANOW, &tty) != 0)
	{
    		printf("Error %i from tcsetattr: %s\n", errno, strerror(errno));
	}

	int x = write(serial_port, array, sizeof(array));
	if ( x < 0 )
	{
		fprintf( stderr, "Failed to write data to PLC - %s\n", strerror( errno ));
		return 1;
	}

	usleep((512+64)*100);

	unsigned char buf [512];	
	int n = read(serial_port, &buf, sizeof(buf));


	printf("Response: ");
	for (int i = 0; i < n; i++)
	{
		printf("%02X ", buf[i]);
	}

	process( array, buf );
	close(serial_port);
	return code;
}



/*
 *  Function:
 *	process() - supporting ioEthernet and ioSerial functions 
 *
 *  Synopsis:
 *	Proccesses response from PLC. Splits buffer into bytes to extract message
 */

unsigned char process( const unsigned char *array, const unsigned char *buf )
{
	uint16_t num = 0;
	uint16_t val = 0;
	
	printf("\nbyte 0 Device address: 0x%02X \n", buf[0]);
	printf("byte 1 Function code: 0x%02X \n", buf[1]);
	printf("byte 2 Byte quantity: 0x%02X \n", buf[2]);
	
	if ( (array[3] == 0x01) && (array[5] == 0x01) ) 
	{
		printf( "byte 3 temp Hi: 0x%02X \n", buf[3] );
		printf( "byte 4 temp Li: 0x%02X \n", buf[4] );
		printf("byte 5 CRC Hi: 0x%02X \n", buf[5]);
		printf("byte 6 CRC Li: 0x%02X \n", buf[6]);
		num = (buf[3] << 8 ) | buf[4];
		num = num / 10;
		printf( "\ntemperature Celsius : %d\n", num );
	} 

	else if ( (array[3] == 0x02) && (array[5] == 0x01) ) 
	{
		printf( "byte 3 humidity Hi: 0x%02X \n", buf[3] );
		printf( "byte 4 humidity Li: 0x%02X \n", buf[4] );
		printf("byte 5 CRC Hi: 0x%02X \n", buf[5]);
		printf("byte 6 CRC Li: 0x%02X \n", buf[6]);
		val = (buf[3] << 8 ) | buf[4];
		val = val / 10;
		printf( "\nHumidity Percent: %d\n", val );
	} 

	else if ( (array[3] == 0x01) && (array[5] == 0x02) )
	{
		printf( "byte 3 temp Hi: 0x%02X \n", buf[3] );
		printf( "byte 4 temp Li: 0x%02X \n", buf[4] );
		printf( "byte 5 humidity Hi: 0x%02X \n", buf[5] );
		printf( "byte 6 humidity Li: 0x%02X \n", buf[6] );
		printf("byte 7 CRC Hi: 0x%02X \n", buf[7]);
		printf("byte 8 CRC Li: 0x%02X \n", buf[8]);
		num = (buf[3] << 8 ) | buf[4];
		val = (buf[5] << 8 ) | buf[6];
		num = num / 10;
		val = val / 10;
		printf( "\ntemperature Celsius : %d\n", num );
		printf( "Humidity Percent: %d\n", val );
	}

	else
	{
		fprintf( stderr, "unreadable packet format%s\n", strerror( errno ));
		return 1;
	}
	return 0;
}



/*
 *  Function:
 *	Usage() - user helper 
 *
 *  Synopsis:
 *	prints out usage of program
 */

int Usage( const int code ) 
{	
	if ( code == 0 ) 
	{
		printf("./app -t 		temperature\n");
		printf("./app -h 		humidity\n");
		printf("./app -c 		continous read temp and humidity\n");
		
	}
	return code;
}



/*
 *  Function:
 *	MODBUS_CRC16() - supporting serial function
 *
 *  Synopsis:
 *	calculates CRC of serial message
 */

static uint16_t MODBUS_CRC16( const unsigned char *buf, unsigned int len )
{
	static const uint16_t table[256] = {
	0x0000, 0xC0C1, 0xC181, 0x0140, 0xC301, 0x03C0, 0x0280, 0xC241,
	0xC601, 0x06C0, 0x0780, 0xC741, 0x0500, 0xC5C1, 0xC481, 0x0440,
	0xCC01, 0x0CC0, 0x0D80, 0xCD41, 0x0F00, 0xCFC1, 0xCE81, 0x0E40,
	0x0A00, 0xCAC1, 0xCB81, 0x0B40, 0xC901, 0x09C0, 0x0880, 0xC841,
	0xD801, 0x18C0, 0x1980, 0xD941, 0x1B00, 0xDBC1, 0xDA81, 0x1A40,
	0x1E00, 0xDEC1, 0xDF81, 0x1F40, 0xDD01, 0x1DC0, 0x1C80, 0xDC41,
	0x1400, 0xD4C1, 0xD581, 0x1540, 0xD701, 0x17C0, 0x1680, 0xD641,
	0xD201, 0x12C0, 0x1380, 0xD341, 0x1100, 0xD1C1, 0xD081, 0x1040,
	0xF001, 0x30C0, 0x3180, 0xF141, 0x3300, 0xF3C1, 0xF281, 0x3240,
	0x3600, 0xF6C1, 0xF781, 0x3740, 0xF501, 0x35C0, 0x3480, 0xF441,
	0x3C00, 0xFCC1, 0xFD81, 0x3D40, 0xFF01, 0x3FC0, 0x3E80, 0xFE41,
	0xFA01, 0x3AC0, 0x3B80, 0xFB41, 0x3900, 0xF9C1, 0xF881, 0x3840,
	0x2800, 0xE8C1, 0xE981, 0x2940, 0xEB01, 0x2BC0, 0x2A80, 0xEA41,
	0xEE01, 0x2EC0, 0x2F80, 0xEF41, 0x2D00, 0xEDC1, 0xEC81, 0x2C40,
	0xE401, 0x24C0, 0x2580, 0xE541, 0x2700, 0xE7C1, 0xE681, 0x2640,
	0x2200, 0xE2C1, 0xE381, 0x2340, 0xE101, 0x21C0, 0x2080, 0xE041,
	0xA001, 0x60C0, 0x6180, 0xA141, 0x6300, 0xA3C1, 0xA281, 0x6240,
	0x6600, 0xA6C1, 0xA781, 0x6740, 0xA501, 0x65C0, 0x6480, 0xA441,
	0x6C00, 0xACC1, 0xAD81, 0x6D40, 0xAF01, 0x6FC0, 0x6E80, 0xAE41,
	0xAA01, 0x6AC0, 0x6B80, 0xAB41, 0x6900, 0xA9C1, 0xA881, 0x6840,
	0x7800, 0xB8C1, 0xB981, 0x7940, 0xBB01, 0x7BC0, 0x7A80, 0xBA41,
	0xBE01, 0x7EC0, 0x7F80, 0xBF41, 0x7D00, 0xBDC1, 0xBC81, 0x7C40,
	0xB401, 0x74C0, 0x7580, 0xB541, 0x7700, 0xB7C1, 0xB681, 0x7640,
	0x7200, 0xB2C1, 0xB381, 0x7340, 0xB101, 0x71C0, 0x7080, 0xB041,
	0x5000, 0x90C1, 0x9181, 0x5140, 0x9301, 0x53C0, 0x5280, 0x9241,
	0x9601, 0x56C0, 0x5780, 0x9741, 0x5500, 0x95C1, 0x9481, 0x5440,
	0x9C01, 0x5CC0, 0x5D80, 0x9D41, 0x5F00, 0x9FC1, 0x9E81, 0x5E40,
	0x5A00, 0x9AC1, 0x9B81, 0x5B40, 0x9901, 0x59C0, 0x5880, 0x9841,
	0x8801, 0x48C0, 0x4980, 0x8941, 0x4B00, 0x8BC1, 0x8A81, 0x4A40,
	0x4E00, 0x8EC1, 0x8F81, 0x4F40, 0x8D01, 0x4DC0, 0x4C80, 0x8C41,
	0x4400, 0x84C1, 0x8581, 0x4540, 0x8701, 0x47C0, 0x4680, 0x8641,
	0x8201, 0x42C0, 0x4380, 0x8341, 0x4100, 0x81C1, 0x8081, 0x4040 };

	uint8_t xor = 0;
	uint16_t crc = 0xFFFF;

	while( len-- )
	{
		xor = (*buf++) ^ crc;
		crc >>= 8;
		crc ^= table[xor];
	}
	return crc;
}



/*
 *  Function:
 *	printMessage() - printing
 *
 *  Synopsis:
 *	prints sent message
 */

void printMessage( const unsigned char *array )
{
	
	printf("Sent message: ");
	for (int i = 0; i < 8; i++)
	{
		printf("%02X ", array[i]);
	}
	printf("\n\nbyte 0 Device address: 0x%02X \n", array[0] );
	printf("byte 1 Function code: 0x%02X \n", array[1] );
	printf("byte 2,3 Inital address: 0x%02X%02X \n", array[2], array[3] );
	printf("byte 4,5 Register Value: 0x%02X%02X \n", array[4], array[5] );
	printf("byte 6,7 CRC verification: 0x%02X%02X \n\n", array[6], array[7] );

}



