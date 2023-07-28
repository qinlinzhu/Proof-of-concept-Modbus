#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <netdb.h>
#include <unistd.h>
#include <sys/socket.h>
#include <errno.h>



int ioEthernet();
unsigned char process( const unsigned char *buf );
static int Connect( char *hostname, int port );
int Usage( const int code );
void printMessage( const unsigned char *array );



int port  = 502;
char *hostname = "192.168.250.20";



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
 *	Creates modbus data, writes data to PLC, reads response from power supply
 */

int ioEthernet()
{
	int fd;
	int code = 0;
	unsigned char tcpArray[] = {0x00, 0x01, 0x00, 0x00, 0x00, 0x06, 0xFF, 0x03, 0x00, 0x00, 0x00, 0x0A};
	int size = sizeof( tcpArray );
	
	printMessage( tcpArray );

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

	process( buf );
	close( fd );
	return code;
}



/*
 *  Function:
 *	process() - supporting ioEthernet functions 
 *
 *  Synopsis:
 *	Proccesses response from Power supply. Splits buffer into bytes to extract message
 */

unsigned char process( const unsigned char *buf )
{
	int binaryNum[16];
	
	float outVoltage = 0;
	float outCurrent = 0;
	float peakCurrent = 0;
	float yReplace = 0;
	float pReplace = 0;
	uint32_t totalTime = 0;	
	uint32_t conTime = 0;

	printf("\n\nbyte 0-6 MBAP Header \n");
	printf("byte 0,1 Transaction Identifer: 0x%02X%02X \n", buf[0], buf[1]);
	printf("byte 2,3 inital address: 0x%02X%02X \n", buf[2], buf[3]);
	printf("byte 4,5 Number of valid bytes: 0x%02X%02X \n", buf[4], buf[5]);
	printf("byte 6 Substation address: 0x%02X \n", buf[6]);
	printf("byte 7 Function code: 0x%02X \n", buf[7]);
	printf("byte 8 target address register: 0x%02X \n", buf[8]);
	printf("byte 9 S8VK-X status: 0x%02X%02X \n", buf[9], buf[10] );
	printf("byte 10 Output voltage: 0x%02X%02X \n", buf[11], buf[12] );
	printf("byte 11 Output current: 0x%02X%02X \n", buf[13], buf[14] );
	printf("byte 12 Peak hold current: 0x%02X%02X \n", buf[15], buf[16] );
	printf("byte 13 Years until replacement: 0x%02X%02X \n", buf[17], buf[18] );
	printf("byte 14 Percentage until replacement: 0x%02X%02X \n", buf[19], buf[20] );
	printf("byte 15,16 Total run time: 0x%02X%02X%02X%02X \n", buf[21], buf[22], buf[23], buf[24] );
	printf("byte 17,18 Continuous run time: 0x%02X%02X%02X%02X \n\n", buf[25], buf[26], buf[27], buf[28] ); 

	// Hex to binary conversion
	uint16_t status = ( buf[10] << 8 ) | buf[9]; 
	for ( int i = 0; i < 16; i++ )
		binaryNum[i] = status & (1U << i ) ? 1 : 0;
	
	outVoltage = ( buf[11] << 8 ) | buf[12];
	outVoltage = outVoltage / 100;	
	
	outCurrent = ( buf[13] << 8 ) | buf[14];
	outCurrent = outCurrent / 100;

	peakCurrent = ( buf[15] << 8 ) | buf[16];
	peakCurrent = peakCurrent / 100;	

	yReplace = ( buf[17] << 8 ) | buf[18];
	yReplace = yReplace / 10;

	pReplace = ( buf[19] << 8 ) | buf[20];
	pReplace = 100 - pReplace / 10;

	conTime = ( buf[25] << 24 ) | ( buf[26] << 16 ) | ( buf[27] << 8 ) | buf[28];
	totalTime = ( buf[21] << 24 ) | ( buf[22] << 16 ) | ( buf[23] << 8 ) | buf[24];
	
	printf("\nS8VK-X status: ");

	printf("\n Bit positions 5 to 7 and 10 to 15 are reserved\n");
	printf(" 0  Memory error: %d\n", binaryNum[8] );
	printf(" 1  Product overheat abnormality: %d \n", binaryNum[7]);
	printf(" 2  Current measurement error: %d \n", binaryNum[6]);
	printf(" 3  Voltage measurement error: %d \n", binaryNum[5]);
	printf(" 4  Overheating alarm: %d \n", binaryNum[4]);
	printf(" 5  Reserved: - \n");
	printf(" 6  Reserved: - \n");
	printf(" 7  Reserved: - \n");
	printf(" 8  Years until the replacement reached FUL: %d \n", binaryNum[1]);
	printf(" 9  Years until the replacement reached HLF: %d \n", binaryNum[0]);
	printf(" 10 Reserved: - \n");
	printf(" 11 Reserved: - \n");
	printf(" 12 Reserved: - \n");
	printf(" 13 Reserved: - \n");
	printf(" 14 Reserved: - \n");
	printf(" 15 Reserved: - \n\n");
	
	printf("Output voltage: %.2f V \n", outVoltage);
	printf("Output current: %.2f A \n", outCurrent );
	printf("Peak hold current: %.2f A \n", peakCurrent );
	printf("Years until replacement: %.1f years \n", yReplace );
	printf("Percentage until replacement: %.1f%% \n", pReplace );
	printf("Total run time: %d hours \n", totalTime ); 
	printf("Continuous run time: %d minutes \n", conTime ); 
	
	return 0;
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
		printf("./app -P portname 	change ethernet port\n");
		printf("./app -H address 	change ethernet address\n");
	}
	return code;
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

