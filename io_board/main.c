#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stdbool.h>
#include <netdb.h>
#include <unistd.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <openssl/ssl.h>
#include <errno.h>
#include <termios.h>
#include <fcntl.h> 


void printRegister( const char *label, const unsigned char reg, const int offset );
int ioEthernet();
int ioSerial();
int Usage( const int code ) ;
unsigned char process( const bool ethernet, const unsigned char *buf );
static int Connect( char *hostname, int port );
void printMessage( const bool ethernet, const unsigned char *array);
static uint16_t MODBUS_CRC16( const unsigned char *buf, unsigned int len );



int port  = 8080;
char *hostname = "192.168.1.80";
int debug = 0;



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
	
	while ((c = getopt(argc, argv, "seP:hH:")) != -1) 
	{
		
		switch (c) 
		{
			case 'h':
				code = Usage( 0 );
				break;

			case 'H':
				hostname = optarg;
				break;

			case 'P':
				port = atoi(optarg);
				break;

			case 'e':
				code = ioEthernet() ;
				break;

			case 's':
				code = ioSerial() ;
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
 *	ioEthernet() - process TCP ethernet modbus
 *
 *  Synopsis:
 *	Creates modbus data, writes data to PLC, reads response from PLC
 */

int ioEthernet()
{
	int fd;
	int code = 0;
	unsigned char tcpArray[] = {0x00, 0x01, 0x00, 0x00, 0x00, 0x06, 0x01, 0x03, 0x00, 0x04, 0x00, 0x02};
	int size = sizeof(tcpArray);
	
	printMessage( 1, tcpArray);

	if (( fd = Connect( hostname, port )) < 0 )
	{
		fprintf( stderr, "Failed to connect to \n" );
		return 1;
	}
	const int BUF = 10000;
	unsigned char buf[ BUF + 1 ];
	
	int x = write( fd, tcpArray, size);
	if ( x != size )
	{
		fprintf( stderr, "Failed to write data to PLC - %s\n", strerror( errno ));
		return 1;
	}
	
	int len = 0;
	x = read( fd, &buf[ len ], 512);
	if ( x < 0 )
	{
		fprintf( stderr, "Failed to read data from PA - %s\n", strerror( errno ));
		return 1;
	}
	
	else if ( len + x >= BUF -1 )
	{
		fprintf( stderr, "Data length returned by PA exceeds buffer length - %s\n", strerror( errno ));
		return 1;
	}

	else if ( len >= BUF - 1)
 	{
		fprintf( stderr, "Data length returned by PA exceeds buffer length - %s\n", strerror( errno ));
		return 1;
	}

	buf[len] = 0;

	printf("\nResponse: ");
	for (int i = 0; i < x; i++)
	{
		printf("%02X ", buf[i]);
	}

	process( 1, buf );
	close( fd );
	return code;
}



/*
 *  Function:
 *	ioSerial() - process serial RS232 modbus
 *
 *  Synopsis:
 *	Creates modbus data, writes data to PLC, reads response from PLC
 */

int ioSerial()
{
	int code = 0;
	unsigned char serialArray[8] = {0x01, 0x03, 0x00, 0x00, 0x00, 0x06};
	uint16_t crc =  MODBUS_CRC16(serialArray, 6);
	serialArray[6] = (crc >> 0) & 0xFF;
	serialArray[7] = (crc >> 8) & 0xFF;
	
	printMessage( 0, serialArray);

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

	int x = write(serial_port, serialArray, sizeof(serialArray));
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

	process( 0, buf );
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

unsigned char process( const bool ethernet, const unsigned char *buf )
{
	if ( ethernet )
	{
		printf("\n\nbyte 0-6 MBAP Header \n");
		printf("byte 0,1 Transaction Identifer: 0x%02X%02X \n", buf[0], buf[1]);
		printf("byte 2,3 inital address: 0x%02X%02X \n", buf[2], buf[3]);
		printf("byte 4,5 Number of valid bytes: 0x%02X%02X \n", buf[4], buf[5]);
		printf("byte 6 Substation address: 0x%02X \n", buf[6]);
		printf("byte 7 Function code: 0x%02X \n", buf[7]);
		printf("byte 8 target address register: 0x%02X \n", buf[8]);
		printf("byte 9-12 Input state: \n\n");

		printRegister( "buf[10]", buf[10], 1 );
		printRegister( "buf[09]", buf[9], 9 );
		printRegister( "buf[12]", buf[12], 17 );
		printRegister( "buf[11]", buf[11], 25 );
	}
	else	// from RS232 coms
	{
	
		printf("\nbyte 0 Substation address: 0x%02X \n", buf[0]);
		printf("byte 1 Function code: 0x%02X \n", buf[1]);
		printf("byte 2 Byte quantity: 0x%02X \n", buf[2]);
		printf("byte 3-10 Blank \n");
		printf("byte 11-14 Input state: \n\n");

		printRegister( "buf[12]", buf[12], 1 );
		printRegister( "buf[11]", buf[11], 9 );
		printRegister( "buf[14]", buf[14], 17 );
		printRegister( "buf[13]", buf[13], 25 );

		printf("\nbyte 15, 16 CRC verification: 0x%02X%02X \n", buf[15],buf[16]);
	}
	return 0;
}



/*
 *  Function:
 *	printRegister() - supporting process function
 *
 *  Synopsis:
 *	prints out input bytes extracted from process function
 */

void printRegister( const char *label, const unsigned char reg, const int offset )
{
	printf( "%s: 0x%02X ", label, (unsigned)reg );
        for ( int j = 0; j < 8; j++ )
        {
		printf("%2d:", j + offset);
		if ( reg & ( 1L << j ))
			printf( "+ " );
		else
			printf( "- " );
        }
        printf( "\n" );
}



static int Connect( char *hostname, int port ) 
{
	struct sockaddr_in addr;
	struct hostent *hp;
	int fd;

	if (( hp = gethostbyname( hostname )) == NULL )
	{
		char *s;
		switch( h_errno )
		{
			case HOST_NOT_FOUND:	s = "unknown host";		break;
			case TRY_AGAIN:		s = "timeout on lookup";	break;
			case NO_RECOVERY:	s = "lookup failure";		break;
			case NO_DATA:		s = "incomplete configuration";	break;
			default:		s = "unknown error";		break;
		}

		fprintf( stderr, "Host name lookup failed - %s\n", s );
		return -1;
	}

	if (( fd = socket( AF_INET, SOCK_STREAM, 0 )) < 0 )
	{
		perror( "failed to secure socket" );
		return -1;
	}

	memset( &addr, 0, sizeof( addr ));
	addr.sin_family = AF_INET;
	addr.sin_port = htons( port );
	memmove((void *)&addr.sin_addr, (void *)hp->h_addr, hp->h_length );

	if ( connect( fd, (struct sockaddr*)&addr, sizeof(addr)) < 0 )
	{
		perror( "failed to connect socket" );
		close( fd );
		return -1;
	}
	return fd;
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
		printf("./app -e 		for ethernet TCP Modbus\n");
		printf("./app -s 		for serial Modbus\n");
		printf("./app -P portname 	change ethernet port\n");
		printf("./app -H address 	change ethernet address\n");
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

void printMessage( const bool ethernet, const unsigned char *array )
{
	if ( ethernet )
	{
		printf("Sent message: ");
		for (int i = 0; i < 12; i++)
		{
			printf("%02X ", array[i]);
		}
		printf("\n\nbyte 0-6 MBAP Header \n");
		printf("byte 0,1 Transaction Identifer: 0x%02X%02X \n", array[0], array[1]);
		printf("byte 2,3 Protocol Identifier: 0x%02X%02X \n", array[2], array[3]);
		printf("byte 4,5 Message Length: 0x%02X%02X \n", array[4], array[5]);
		printf("byte 6 Substation address: 0x%02X \n", array[6]);
		printf("byte 7 Function code: 0x%02X \n", array[7]);
		printf("byte 8,9 Inital address: 0x%02X%02X \n", array[8], array[9]);
		printf("byte 10,11 Register Value: 0x%02X%02X \n", array[10], array[11]);
	}
	else	// serial
	{
		printf("Sent message: ");
		for (int i = 0; i < 8; i++)
		{
			printf("%02X ", array[i]);
		}
		printf("\n\nbyte 0 Substation address: 0x%02X \n", array[0] );
		printf("byte 1 Function code: 0x%02X \n", array[1] );
		printf("byte 2,3 Inital address: 0x%02X%02X \n", array[2], array[3] );
		printf("byte 4,5 Register Value: 0x%02X%02X \n", array[4], array[5] );
		printf("byte 6,7 CRC verification: 0x%02X%02X \n\n", array[6], array[7] );
	}
}



