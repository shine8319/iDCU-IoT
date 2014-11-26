#include<stdio.h>
#include<string.h>
#include<fcntl.h>
#include<unistd.h>
#include "./include/uart_init.h"


int main(int argc, char **argv)
{

	char *devname;
	char *temp = "Pass!!! Leading the future challenges....\n";
	int fd;
	int rtrn;

	if( argc != 3 )
	{
		fprintf(stderr, "Usage : %s <Open Device> <Baud Rate:5(9600),6(19200),8(57600),9(115200)>\n", argv[0]);
		return -1;
	}

	switch( argc )
	{
		case 2:
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

	//rtrn = tty_raw(fd, atoi(argv[2]), FLOWCONTROL, DATABIT, PARITYBIT, STOPBIT); 
	rtrn = tty_raw(fd, 9, FLOWCONTROL, DATABIT, PARITYBIT, STOPBIT); 
	if( rtrn != 0 )	{
		printf("%s Setting error!!\n", devname );
		close(fd);
		return -1;
	}

	while(1)
	{
		write( fd, temp, strlen( temp ));
		printf("==> %s\n", temp);
		sleep(2);
	}
	close( fd );


	return 0;
}






	
