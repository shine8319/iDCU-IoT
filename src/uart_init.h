/***** Serial Setup *********/
//#define BAUDRATE 	9		//  115200
//#define BAUDRATE 	8		// 57600 
#define BAUDRATE 	5		// 9600 
#define FLOWCONTROL 'n'
#define DATABIT 	8
#define PARITYBIT 	1
#define STOPBIT 	1
/****************************/
int tty_raw(int fd, int baud, char flowc, unsigned char databits, unsigned char parity, unsigned char stopbits);

void tty_reset(void);
