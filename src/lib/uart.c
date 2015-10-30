#include <stdio.h>
#include <string.h>
#include <termio.h>
#include <unistd.h>
#include <fcntl.h>
#include <sys/stat.h> 

#include "../include/iDCU.h"

int save_fd = -1;	// tty setting
struct termios save_tios;	// for tty setting

void tty_reset(void)
{
    if (save_fd >= 0) {
	fprintf(stderr, "Reseting device attributes\n");
	tcsetattr(save_fd, TCSAFLUSH, &save_tios);
	save_fd = -1;
    }
}

int tty_raw(int fd, char *baud, char flowc, char databits, char parity, char stopbits)
{
    struct termios tios;
    int baudrate;

    if (tcgetattr(fd, &save_tios) < 0) {
	perror("Cannot get tty attributes");
	return -1;
    }

    // make sure the device will be reset on exit
    save_fd = fd;
    if ( atexit(tty_reset) < 0 ) {
	fprintf(stderr, "Cannot arrange for atexit handler\n");
	return -1;
    }

    tios = save_tios;

    // raw device sttings
    cfmakeraw(&tios);

    printf("fd %d, baud%s, parity %c, databits %c, stopbits %c, flowc %c\n",
	    fd, baud, parity, databits, stopbits, flowc);

    // character size (databits)
    tios.c_cflag &= ~(CSIZE);
    switch (databits) {
	case '5':
	    tios.c_cflag |= CS5;//uint
	    break;

	case '6':
	    tios.c_cflag |= CS6;
	    break;

	case '7':
	    tios.c_cflag |= CS7;
	    break;

	case '8':
	    printf("databits %c\n", databits);
	    tios.c_cflag |= CS8;
	    break;

	default:
	    fprintf(stderr,"Invalid databits: %d\n", databits);
	    return -1;
    }

    // parity (E = even, O = odd, N = no)
    switch (parity) {
	case 'E':
	    tios.c_cflag |= PARENB;
	    tios.c_cflag &= ~(PARODD);
	    break;

	case 'O':
	    tios.c_cflag |= PARENB;
	    tios.c_cflag |= PARODD;
	    break;

	case 'N':
	    tios.c_cflag &= ~(PARENB);
	    printf("parity %c\n", parity );
	    break;

	default:
	    fprintf(stderr, "Invalid parity: %c\n", parity);
	    return -1;
    }

    // stopbits (1 or 2)
    switch (stopbits) {
	case '1':
	    tios.c_cflag &= ~(CSTOPB);
	    printf("stopbits %c\n", stopbits);
	    break;

	    /*
	       case '2':		// 1.5 stopbit is not available
	       fprintf(stderr,"Invalid stopbits: %d\n", stopbits);
	       return -1;
	     */

	case '2':
	    tios.c_cflag |= CSTOPB;
	    break;

	default:
	    fprintf(stderr,"Invalid stopbits: %d\n", stopbits);
	    return -1;
    }   

    // flow control type (h = RTC/CTS, s = XON/XOFF, n = none)
    switch (flowc) {
	case 'H':
	    tios.c_cflag |= CRTSCTS;
	    tios.c_iflag &= ~(IXON | IXOFF);
	    break;

	case 'S':
	    tios.c_cflag &= ~(CRTSCTS);
	    tios.c_iflag |= (IXON | IXOFF);
	    break;

	case 'N':
	    tios.c_cflag &= ~(CRTSCTS);
	    tios.c_iflag &= ~(IXON | IXOFF);
	    printf("flowc %c\n", flowc);
	    break;

	default:
	    fprintf(stderr,"Invalid flow control: %c\n", flowc);
	    return -1;
    }

    //tios.c_cc[VMIN] = 1;//16
    //tios.c_cc[VTIME] = 1;//17

    if (strcmp(baud,"1200") == 0) 
    {
	baudrate = B1200;
    }
    else if (strcmp(baud,"2400") == 0) 
    {
	baudrate = B2400;
    }
    else if (strcmp(baud,"4800") == 0) 
    {
	baudrate = B4800;
    }
    else if (strcmp(baud,"9600") == 0) 
    {
	baudrate = B9600;
    }
    /*
       else if (strcmp(baud,"14400") == 0) 
       {
       baudrate = B14400;
       }
     */
    else if (strcmp(baud,"19200") == 0) 
    {
	baudrate = B19200;
    }
    else if (strcmp(baud,"38400") == 0) 
    {
	baudrate = B38400;
    }
    else if (strcmp(baud,"57600") == 0) 
    {
	baudrate = B57600;
    }
    else if (strcmp(baud,"115200") == 0) 
    {
	baudrate = B115200;
    }
    else if (strcmp(baud,"230400") == 0) 
    {
	baudrate = B230400;
    }
    else
    {
	fprintf(stderr,"Invalid baud-rate: %s\n", baud);
	return -1;
    }

    if ( cfsetospeed(&tios, baudrate) < 0 ) {
	perror("Cannot set output speed");
	return -1;
    }

    if ( cfsetispeed(&tios, baudrate) < 0 ) {
	perror("Cannot set input speed");
	return -1;
    }

    // set the new attributes. let them take effect now
    if ( tcsetattr(fd, TCSANOW, &tios) < 0 ) {
	perror("Cannot set device attributes");
	return -1;
    }

    // flush the device, of any remains before the switch
    if ( tcflush(fd, TCIOFLUSH) < 0 ) {
	perror("Cannot flush the device");
	return -1;
    }

    return 0;
}



int OpenSerialV2(char *port, char *baud, char parity, char databits, char stopbits )
{
    //-------------------------
    //----- SETUP USART 0 -----
    //-------------------------
    //At bootup, pins 8 and 10 are already set to UART0_TXD, UART0_RXD (ie the alt0 function) respectively
    int uart0_filestream = -1;

    long mBAUD;                      // derived baud rate from command line
    long mDATABITS;
    long mSTOPBITS;
    long mPARITYON;
    long mPARITY;

    //printf("baud %s, partiy %c, databits %c, stopbits %c\n",baud, parity, databits, stopbits );

    //OPEN THE UART
    //The flags (defined in fcntl.h):
    //  Access modes (use 1 of these):
    //	O_RDONLY - Open for reading only.
    //	O_RDWR - Open for reading and writing.
    //	O_WRONLY - Open for writing only.
    //
    //  O_NDELAY / O_NONBLOCK (same function) - Enables nonblocking mode. When set read requests on the file can return immediately with a failure status
    //					    if there is no input immediately available (instead of blocking). Likewise, write requests can also return
    //					    immediately with a failure status if the output can't be written immediately.
    //
    //  O_NOCTTY - When set and path identifies a terminal device, open() shall not cause the terminal device to become the controlling terminal for the process.
    uart0_filestream = open(port, O_RDWR | O_NOCTTY | O_NDELAY);	    //Open in non blocking read/write mode
    if (uart0_filestream == -1)
    {
	//ERROR - CAN'T OPEN SERIAL PORT
	printf("Error - Unable to open UART.  Ensure it is not in use by another application\n");
	return -1;
    }

    //CONFIGURE THE UART
    //The flags (defined in /usr/include/termios.h - see http://pubs.opengroup.org/onlinepubs/007908799/xsh/termios.h.html):
    //	Baud rate:- B1200, B2400, B4800, B9600, B19200, B38400, B57600, B115200, B230400, B460800, B500000, B576000, B921600, B1000000, B1152000, B1500000, B2000000, B2500000, B3000000, B3500000, B4000000
    //	CSIZE:- CS5, CS6, CS7, CS8
    //	CLOCAL - Ignore modem status lines
    //	CREAD - Enable receiver
    //	IGNPAR = Ignore characters with parity errors
    //	ICRNL - Map CR to NL on input (Use for ASCII comms where you want to auto correct end of line characters - don't use for bianry comms!)
    //	PARENB - Parity enable
    //	PARODD - Odd parity (else even)
    struct termios options;
    tcgetattr(uart0_filestream, &options);

    if (strcmp(baud,"1200") == 0) 
    {
	//options.c_cflag = B1200;
	mBAUD = B1200;
    }
    else if (strcmp(baud,"2400") == 0) 
    {
	mBAUD = B2400;
    }
    else if (strcmp(baud,"4800") == 0) 
    {
	mBAUD = B4800;
    }
    else if (strcmp(baud,"9600") == 0) 
    {
	mBAUD = B9600;
    }
    /*
       else if (strcmp(baud,"14400") == 0) 
       {
       options.c_cflag = B14400;
       }
     */
    else if (strcmp(baud,"19200") == 0) 
    {
	mBAUD = B19200;
    }
    else if (strcmp(baud,"38400") == 0) 
    {
	mBAUD = B38400;
    }
    else if (strcmp(baud,"57600") == 0) 
    {
	mBAUD = B57600;
    }
    else if (strcmp(baud,"115200") == 0) 
    {
	mBAUD = B115200;
    }
    else if (strcmp(baud,"230400") == 0) 
    {
	mBAUD = B230400;
    }
    else
    {
	fprintf(stderr,"Invalid baud-rate: %s\n", baud);
	return -1;
    }

    switch (databits) {
	case '5':
	    //options.c_cflag |= CS5;
	    mDATABITS = CS5;
	    break;

	case '6':
	    mDATABITS = CS6;
	    break;

	case '7':
	    mDATABITS = CS7;
	    break;

	case '8':
	    //printf("databits %c\n", databits);
	    mDATABITS = CS8;
	    break;

	default:
	    fprintf(stderr,"Invalid databits: %d\n", databits);
	    return -1;
    }

    switch (stopbits)
    {
	case 1:
	default:
	    mSTOPBITS = 0;
	    break;
	case 2:
	    mSTOPBITS = CSTOPB;
	    break;
    }  //end of switch stop bits

    switch (parity)
    {
	case 'N':
	default:                       //none
	    mPARITYON = 0;
	    mPARITY = 0;
	    //printf("parity none\n");
	    break;
	case 'O':                        //odd
	    mPARITYON = PARENB;
	    mPARITY = PARODD;
	    break;
	case 'E':                        //even
	    mPARITYON = PARENB;
	    mPARITY = 0;
	    break;
    }  //end of switch parity


    //options.c_cflag = B115200 | CS8 | CLOCAL | CREAD;	//<Set baud rate
    options.c_cflag = mBAUD | CRTSCTS | mDATABITS | mSTOPBITS | mPARITYON | mPARITY | CLOCAL | CREAD;
    options.c_iflag = IGNPAR;
    options.c_oflag = 0;
    options.c_lflag = 0;
    tcflush(uart0_filestream, TCIFLUSH);
    tcsetattr(uart0_filestream, TCSANOW, &options);


    return uart0_filestream;
}


//int setupUSART()
int OpenSerial(char *baud, char parity, char databits, char stopbits )
{
    //-------------------------
    //----- SETUP USART 0 -----
    //-------------------------
    //At bootup, pins 8 and 10 are already set to UART0_TXD, UART0_RXD (ie the alt0 function) respectively
    int uart0_filestream = -1;

    long mBAUD;                      // derived baud rate from command line
    long mDATABITS;
    long mSTOPBITS;
    long mPARITYON;
    long mPARITY;

    //printf("baud %s, partiy %c, databits %c, stopbits %c\n",baud, parity, databits, stopbits );

    //OPEN THE UART
    //The flags (defined in fcntl.h):
    //  Access modes (use 1 of these):
    //	O_RDONLY - Open for reading only.
    //	O_RDWR - Open for reading and writing.
    //	O_WRONLY - Open for writing only.
    //
    //  O_NDELAY / O_NONBLOCK (same function) - Enables nonblocking mode. When set read requests on the file can return immediately with a failure status
    //					    if there is no input immediately available (instead of blocking). Likewise, write requests can also return
    //					    immediately with a failure status if the output can't be written immediately.
    //
    //  O_NOCTTY - When set and path identifies a terminal device, open() shall not cause the terminal device to become the controlling terminal for the process.
    uart0_filestream = open("/dev/ttyAMA0", O_RDWR | O_NOCTTY | O_NDELAY);	    //Open in non blocking read/write mode
    if (uart0_filestream == -1)
    {
	//ERROR - CAN'T OPEN SERIAL PORT
	printf("Error - Unable to open UART.  Ensure it is not in use by another application\n");
    }

    //CONFIGURE THE UART
    //The flags (defined in /usr/include/termios.h - see http://pubs.opengroup.org/onlinepubs/007908799/xsh/termios.h.html):
    //	Baud rate:- B1200, B2400, B4800, B9600, B19200, B38400, B57600, B115200, B230400, B460800, B500000, B576000, B921600, B1000000, B1152000, B1500000, B2000000, B2500000, B3000000, B3500000, B4000000
    //	CSIZE:- CS5, CS6, CS7, CS8
    //	CLOCAL - Ignore modem status lines
    //	CREAD - Enable receiver
    //	IGNPAR = Ignore characters with parity errors
    //	ICRNL - Map CR to NL on input (Use for ASCII comms where you want to auto correct end of line characters - don't use for bianry comms!)
    //	PARENB - Parity enable
    //	PARODD - Odd parity (else even)
    struct termios options;
    tcgetattr(uart0_filestream, &options);

    if (strcmp(baud,"1200") == 0) 
    {
	//options.c_cflag = B1200;
	mBAUD = B1200;
    }
    else if (strcmp(baud,"2400") == 0) 
    {
	mBAUD = B2400;
    }
    else if (strcmp(baud,"4800") == 0) 
    {
	mBAUD = B4800;
    }
    else if (strcmp(baud,"9600") == 0) 
    {
	mBAUD = B9600;
    }
    /*
       else if (strcmp(baud,"14400") == 0) 
       {
       options.c_cflag = B14400;
       }
     */
    else if (strcmp(baud,"19200") == 0) 
    {
	mBAUD = B19200;
    }
    else if (strcmp(baud,"38400") == 0) 
    {
	mBAUD = B38400;
    }
    else if (strcmp(baud,"57600") == 0) 
    {
	mBAUD = B57600;
    }
    else if (strcmp(baud,"115200") == 0) 
    {
	mBAUD = B115200;
    }
    else if (strcmp(baud,"230400") == 0) 
    {
	mBAUD = B230400;
    }
    else
    {
	fprintf(stderr,"Invalid baud-rate: %s\n", baud);
	return -1;
    }

    switch (databits) {
	case '5':
	    //options.c_cflag |= CS5;
	    mDATABITS = CS5;
	    break;

	case '6':
	    mDATABITS = CS6;
	    break;

	case '7':
	    mDATABITS = CS7;
	    break;

	case '8':
	    //printf("databits %c\n", databits);
	    mDATABITS = CS8;
	    break;

	default:
	    fprintf(stderr,"Invalid databits: %d\n", databits);
	    return -1;
    }

    switch (stopbits)
    {
	case 1:
	default:
	    mSTOPBITS = 0;
	    break;
	case 2:
	    mSTOPBITS = CSTOPB;
	    break;
    }  //end of switch stop bits

    switch (parity)
    {
	case 'N':
	default:                       //none
	    mPARITYON = 0;
	    mPARITY = 0;
	    //printf("parity none\n");
	    break;
	case 'O':                        //odd
	    mPARITYON = PARENB;
	    mPARITY = PARODD;
	    break;
	case 'E':                        //even
	    mPARITYON = PARENB;
	    mPARITY = 0;
	    break;
    }  //end of switch parity


    //options.c_cflag = B115200 | CS8 | CLOCAL | CREAD;	//<Set baud rate
    options.c_cflag = mBAUD | CRTSCTS | mDATABITS | mSTOPBITS | mPARITYON | mPARITY | CLOCAL | CREAD;
    options.c_iflag = IGNPAR;
    options.c_oflag = 0;
    options.c_lflag = 0;
    tcflush(uart0_filestream, TCIFLUSH);
    tcsetattr(uart0_filestream, TCSANOW, &options);


    return uart0_filestream;
}

/*
   int OpenSerial(void)
   {
   int fd; // File descriptor
   struct termios newtio;

   int rtrn;

   fd = open( "/dev/ttyAMA0", O_RDWR | O_NOCTTY);
   if(fd < 0)
   {
   printf("ttyAMA0 Open Fail.\n");
   return -1;
   }
   memset(&newtio, 0, sizeof(newtio));
   newtio.c_iflag = IGNPAR; // Parity error 문자 바이트 무시
   newtio.c_oflag = 0; // 2
   newtio.c_cflag = CS8|CLOCAL|CREAD; // 8N1, Local Connection, 문자(char형)수신 가능

   switch(115200) // Baud Rate
   {
   case 115200 : newtio.c_cflag |= B115200;
   break;
   case 57600 : newtio.c_cflag |= B57600;
   break;
   case 38400 : newtio.c_cflag |= B38400;
   break;
   case 19200 : newtio.c_cflag |= B19200;
   break;
   case 9600: newtio.c_cflag |= B9600;
   break;
   default : newtio.c_cflag |= B57600;
   break;
   }
   newtio.c_lflag = 0;	    // Non-Canonical 입력 처리
//newtio.c_cc[VTIME] = 1;// 0.1초 이상 수신이 없으면 timeout. read()함수는 0을 리턴
//newtio.c_cc[VMIN] = 0;

tcflush(fd, TCIFLUSH); // Serial Buffer initialized
tcsetattr(fd, TCSANOW, &newtio); // Modem line initialized & Port setting 종료

return fd;
}

 */
int CloseSerial(int fd)
{
    close(fd);
}

int getDatabit(char databit )
{
    int data = 3;

    switch (databit) {
	case '5':
	    data = 0;
	    break;

	case '6':
	    data = 1;
	    break;

	case '7':
	    data = 2;
	    break;

	case '8':
	    data = 3;
	    break;
    }
    return data;

}

int getBaudrate(char *baud)
{
    int baudrate = 0;

    if (strcmp(baud,"1200") == 0) 
    {
	baudrate = 0;
    }
    else if (strcmp(baud,"2400") == 0) 
    {
	baudrate = 1;
    }
    else if (strcmp(baud,"4800") == 0) 
    {
	baudrate = 2;
    }
    else if (strcmp(baud,"9600") == 0) 
    {
	baudrate = 3;
    }
    /*
       else if (strcmp(baud,"14400") == 0) 
       {
       baudrate = 4;
       }
     */
    else if (strcmp(baud,"19200") == 0) 
    {
	baudrate = 4;
    }
    else if (strcmp(baud,"38400") == 0) 
    {
	baudrate = 5;
    }
    else if (strcmp(baud,"57600") == 0) 
    {
	baudrate = 6;
    }
    else if (strcmp(baud,"115200") == 0) 
    {
	baudrate = 7;
    }
    else if (strcmp(baud,"230400") == 0) 
    {
	baudrate = 8;
    }

    return baudrate;
}


UINT8 getStopbit(UINT8 stopbit)
{
    UINT8 data;

    switch (stopbit)
    {
	case '1':
	default:
	    data = 0;
	    break;
	case '2':
	    data = 1;
	    break;
    }  //end of switch stop bits

    return data;
}


UINT8 getParity(UINT8 parity)
{

    UINT8 data;

    switch (parity)
    {
	case 'N':
	default:                       //none
	    data = 0;
	    break;
	case 'O':                        //odd
	    data = 2;
	    break;
	case 'E':                        //even
	    data = 1;
	    break;
    }  //end of switch parity

    return data;

}

int getConnectMode(char *mode)
{
    int connectMode = 0;

    if (strcmp(mode,"SERVER") == 0) 
    {
	connectMode = 0;
    }
    else if (strcmp(mode,"CLIENT") == 0) 
    {
	connectMode = 1;
    }

    return connectMode;
}

