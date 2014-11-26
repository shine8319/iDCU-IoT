
#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <termio.h>
#include <unistd.h>
#include <fcntl.h>

// Zigbee Coordinator Device Name
#define ZIGBEE	"/dev/ttyUSB1"
// Zigbee Coordinator Baud Rate
#define	BAUDRATE	115200	
#define BUFFER_SIZE		1024

int OpenSerial(void);
int closeSerial(int fd);


int main(int argc, char** argv)
{
	// wifi
	int tty = 0;
	struct termios tio, old_tio;
	int ret;
	char buff[10];

	// usn
	int fd; // Serial Port File Descriptor
	int ReadMsgSize = 0;
	unsigned char DataBuf[BUFFER_SIZE];


	// wifi
	tty = open( "/dev/ttyUSB0", O_RDWR, 0);
	if(tty == -1) {
		perror("open()\n");
		return 0;
	}

	memset(&tio, 0, sizeof(tio)); 
	tio.c_iflag = IGNPAR; 
	//tio.c_cflag = B57600 | HUPCL | CS8 | CLOCAL | CREAD; 
	tio.c_cflag = B115200 | HUPCL | CS8 | CLOCAL | CREAD; 
	tio.c_cc[VMIN] = 10;
	tio.c_cc[VTIME] = 0;
	tcgetattr(tty, &old_tio);
	tcsetattr(tty, TCSANOW, &tio);


	// usn
	fd = OpenSerial();
	memset(DataBuf, 0, BUFFER_SIZE);
	
	while(1)
	{ 
		//read(tty, buff, 10);
		//ReadMsgSize = read(fd, DataBuf, BUFFER_SIZE); 
		ReadMsgSize = read(tty, DataBuf, BUFFER_SIZE); 
		if(ReadMsgSize > 0)
			write(fd, DataBuf, ReadMsgSize);
			//write(tty, DataBuf, ReadMsgSize);
	}
	

	CloseSerial(fd);
	tcsetattr(tty, TCSANOW, &old_tio); 
	close(tty); 
	return 0;
}


int OpenSerial(void)
{
	int fd; // File descriptor
	struct termios newtio;
	
	fd = open(ZIGBEE, O_RDWR | O_NOCTTY);

	if(fd < 0)
	{
		printf("Zigbee Coordinator Open Fail.\n");
		return -1;
	}
	memset(&newtio, 0, sizeof(newtio));
	newtio.c_iflag = IGNPAR; // Parity error 문자 바이트 무시
	newtio.c_oflag = 0; // 2
	newtio.c_cflag = CS8|CLOCAL|CREAD; // 8N1, Local Connection, 문자(char형)수신 가능
	
	switch(BAUDRATE) // Baud Rate
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
	newtio.c_cc[VTIME] = 20;// 2초 이상 수신이 없으면 timeout. read()함수는 0을 리턴
	newtio.c_cc[VMIN] = 1;	// 1-byte 이상의 데이터가 들어 올 때까지 대기
	
	tcflush(fd, TCIFLUSH); // Serial Buffer initialized
	tcsetattr(fd, TCSANOW, &newtio); // Modem line initialized & Port setting 종료

	return fd;
}

int CloseSerial(int fd)
{
	close(fd);
}
