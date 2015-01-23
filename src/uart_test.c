#include<fcntl.h>
#include<unistd.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <arpa/inet.h>
#include <sys/ipc.h>
#include <sys/msg.h>
#include <time.h>
#include <sys/reboot.h>


#include "./uart_init.h"


void *thread_receive(void *arg);
pthread_t thread;
int fd;

int main(int argc, char **argv)
{

	char *devname;
	char *temp = "Pass!!! Leading the future challenges....\n";
	int rtrn;

	if( argc != 3 )
	{
		fprintf(stderr, "Usage : %s <Open Device> <Baud Rate:5(9600),6(19200),8(57600),9(115200)>\n", argv[0]);
		return -1;
	}

	switch( argc )
	{
		case 3:
			devname = argv[1];
			break;
	}
	
	//if( 0 < (fd = open( devname, O_WRONLY | O_CREAT | O_EXCL, 0644)))
	if( 0 < (fd = open( devname, O_RDWR, 0666)))
	{
		printf("%s Open OK!!\n", devname );
	}
	else
	{
		printf("%s Open error!!\n", devname );
	}

	rtrn = tty_raw(fd, atoi(argv[2]), FLOWCONTROL, DATABIT, PARITYBIT, STOPBIT); 
	//rtrn = tty_raw(fd, 9, FLOWCONTROL, DATABIT, PARITYBIT, STOPBIT); 
	if( rtrn != 0 )	{
		printf("%s Setting error!!\n", devname );
		close(fd);
		return -1;
	}

	if( pthread_create(&thread, NULL, &thread_receive, NULL ) == -1 )
	{
		printf("thread error!!\n");
	}


	while(1)
	{
		write( fd, temp, strlen( temp ));
		printf("==> %s\n", temp);
		sleep(5);
	}
	close( fd );


	return 0;
}


void *thread_receive(void *arg)
{

    char recv_buff[1024];
    int rtrn,i;

    while(1)
    {
	rtrn = read( fd, recv_buff, 1024);
	if( rtrn > 0 )
	{
	    for( i = 0; i < rtrn; i++ )
	       printf("%02X ", recv_buff[i]);
	    printf("\n");    
	}

    }
}

 



	
