#include<stdio.h>
#include<asm/termbits.h>
#include "./uart_init.h"

int save_fd = -1;	// tty setting
struct termios save_tios;	// for tty setting

int tty_raw(int fd, int baud, char flowc, unsigned char databits, unsigned char parity, unsigned char stopbits)
{
	struct termios tios;

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

	// character size (databits)
	tios.c_cflag &= ~(CSIZE);
	switch (databits) {
		case 5:
			tios.c_cflag |= CS5;//uint
			break;

		case 6:
			tios.c_cflag |= CS6;
			break;

		case 7:
			tios.c_cflag |= CS7;
			break;

		case 8:
			tios.c_cflag |= CS8;
			break;

		default:
			fprintf(stderr,"Invalid databits: %d\n", databits);
			return -1;
	}

	// parity (E = even, O = odd, N = no)
	switch (parity) {
		case 2:
			tios.c_cflag |= PARENB;
			tios.c_cflag &= ~(PARODD);
			break;

		case 3:
			tios.c_cflag |= PARENB;
			tios.c_cflag |= PARODD;
			break;

		case 1:
			tios.c_cflag &= ~(PARENB);
			break;

		default:
			fprintf(stderr, "Invalid parity: %c\n", parity);
			return -1;
	}

	// stopbits (1 or 2)
	switch (stopbits) {
		case 1:
			tios.c_cflag &= ~(CSTOPB);
			break;

		case 2:		// 1.5 stopbit is not available
			fprintf(stderr,"Invalid stopbits: %d\n", stopbits);
			return -1;

		case 3:
			tios.c_cflag |= CSTOPB;
			break;

		default:
			fprintf(stderr,"Invalid stopbits: %d\n", stopbits);
			return -1;
	}   

	// flow control type (h = RTC/CTS, s = XON/XOFF, n = none)
	switch (flowc) {
		case 'h':
			tios.c_cflag |= CRTSCTS;
			tios.c_iflag &= ~(IXON | IXOFF);
			break;

		case 's':
			tios.c_cflag &= ~(CRTSCTS);
			tios.c_iflag |= (IXON | IXOFF);
			break;

		case 'n':
			tios.c_cflag &= ~(CRTSCTS);
			tios.c_iflag &= ~(IXON | IXOFF);
			break;

		default:
			fprintf(stderr,"Invalid flow control: %c\n", flowc);
			return -1;
	}

	tios.c_cc[VMIN] = 1;//16
	tios.c_cc[VTIME] = 1;//17

	// baud-rate
	switch(baud) {
		case 1: 
			baud = B300;

		case 2:
			baud = B1200;
			break;

		case 3:
			baud = B2400;
			break;

		case 4:
			baud = B4800;
			break;

		case 5:
			baud = B9600;
			break;

		case 6:
			baud = B19200;
			break;

		case 7:
			baud = B38400;
			break;

		case 8:
			baud = B57600;
			break;

		case 9:
			baud = B115200;
			break;

		case 10:
			baud = B230400;
			break;

		default:
			fprintf(stderr,"Invalid baud-rate: %d\n", baud);
			return -1;
	}

	if ( cfsetospeed(&tios, baud) < 0 ) {
		perror("Cannot set output speed");
		return -1;
	}

	if ( cfsetispeed(&tios, baud) < 0 ) {
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



void tty_reset(void)
{
	if (save_fd >= 0) {
		fprintf(stderr, "Reseting device attributes\n");
		tcsetattr(save_fd, TCSAFLUSH, &save_tios);
		save_fd = -1;
	}
}
