
#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <termio.h>
#include <unistd.h>
#include <fcntl.h>
#include <sys/ipc.h>
#include <sys/msg.h>

// Zigbee Coordinator Device Name
#define ZIGBEE	"/dev/ttyUSB1"
// Zigbee Coordinator Baud Rate
#define	BAUDRATE	115200	
#define BUFFER_SIZE		1024
#define		M2M_COMMAND		0
#define		USN_RETURN		1

typedef struct { 
long data_type; 
int data_num; 
char data_buff[BUFFER_SIZE]; 
} t_data; 

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

	// q
	t_data data[2];
	int msqid;

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
	tio.c_cc[VTIME] = 1;
	tio.c_cc[VMIN] = 0;
	tcgetattr(tty, &old_tio);
	tcsetattr(tty, TCSANOW, &tio);


	// usn
	fd = OpenSerial();
	memset(DataBuf, 0, BUFFER_SIZE);
	

	// q
	if( -1 == ( msqid = msgget( (key_t)1234, IPC_CREAT | 0666)))
   	{
		perror( "msgget() 실패");
   	}

	while(1)
	{ 
		//read(tty, buff, 10);
		ReadMsgSize = read(fd, DataBuf, BUFFER_SIZE); 
		if(ReadMsgSize > 0)
		{
			write(tty, DataBuf, ReadMsgSize);
			printf("Serial Recv %d Byte\n", ReadMsgSize);
		}
		else
			printf("Serial timeout %d\n", ReadMsgSize);


		// receive type 1
		if( -1 == msgrcv( msqid, &data[M2M_COMMAND], sizeof( t_data) - sizeof( long), 1, IPC_NOWAIT) )
		{
			printf("Empty Q\n");
		}
		else
		{
			printf( "%d - %s\n", data[M2M_COMMAND].data_num, data[M2M_COMMAND].data_buff);

			  data[USN_RETURN].data_type = 2;   // data_type 는 2
			data[USN_RETURN].data_num = data[M2M_COMMAND].data_num;
			  sprintf( data[USN_RETURN].data_buff, "type=%d, ndx=%d, USN Return Msg", data[USN_RETURN].data_type, data[USN_RETURN].data_num);
			  if ( -1 == msgsnd( msqid, &data[USN_RETURN], sizeof( t_data) - sizeof( long), IPC_NOWAIT))
			  {
				 perror( "msgsnd() 실패");
				 //exit( 1);
			  }


		}

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
	newtio.c_cc[VTIME] = 1;// 0.1초 이상 수신이 없으면 timeout. read()함수는 0을 리턴
	newtio.c_cc[VMIN] = 0;
	
	tcflush(fd, TCIFLUSH); // Serial Buffer initialized
	tcsetattr(fd, TCSANOW, &newtio); // Modem line initialized & Port setting 종료

	return fd;
}

int CloseSerial(int fd)
{
	close(fd);
}
