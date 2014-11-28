#include <stdio.h> 
#include <ctype.h>
#include <stdlib.h> 
#include <string.h> 
#include <unistd.h>
#include <sys/ipc.h>
#include <sys/msg.h>
#include <sys/shm.h>
#include <signal.h>
#include <sys/socket.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <arpa/inet.h>
#include <fcntl.h>
#include <errno.h>
#include <time.h>
#include <termio.h>
#include <sys/wait.h>

/*
#include "./include/sqlite3.h"
#include "./include/M2MManager.h"
#include "./include/configRW.h"
#include "./include/writeLog.h"
#include "./include/TCPSocket.h"
*/
#include "./include/iDCU.h"

#include "./driver/include/PLCS_10.h"
#include "./driver/include/PLCS_12.h"
#include "./driver/include/PLCS_12ex.h"
#include "./driver/include/iDCU_IoT.h"

#include "./include/libpointparser.h"
#include "./include/libDeviceInfoParser.h"


void sig_handler( int signo);

	
void createProcess( DEVICEINFO *device, TAGINFO *tag )
{

    if (strcmp(device->driver,"PLCS10") == 0) 
    {
	 PLCS10(device);
    }
    else if (strcmp(device->driver,"PLCS12") == 0) 
    {
	 PLCS12(device);
    }
    else if (strcmp(device->driver,"PLCS12ex") == 0) 
    {
	 PLCS12ex(device);
    }
    else if (strcmp(device->driver,"iDCU_IoT") == 0) 
    {
	 iDCU_IoT(device);
    }
    else
	printf("no exist~~~~~~\n");

}


int main(int argc, char **argv) { 

    int i;
    pid_t pid;
    int threadCount = 0;
    NODEINFO xmlinfo;
    //xmlinfo = pointparser("/work/smart/device_info.xml");
    xmlinfo = deviceInfoParser("/work/smart/device_info.xml");

    signal( SIGINT, (void *)sig_handler);

    printf("xmlinfo.getPointSize %d\n", xmlinfo.getPointSize);
    for( i = 0; i < xmlinfo.getPointSize; i++ )
    {
	pid = fork();
	if( pid < 0 )
	{
	    perror("fork error : ");
	    writeLog("/work/smart/comm/log/deviceLog", "[CommunicationManager] error fork()" );
	    printf("error process\n");
	}

	if( pid == 0 )
	{
	    createProcess( &xmlinfo.device[i], &xmlinfo.tag[i] );
	    exit(0);
	}
	else
	{
	    printf("pid %d\n", pid );
	    threadCount++;
	    printf("proces Count %d\n", threadCount );
	}

    }

    writeLog("/work/smart/log", "[CommunicationManager] start CommunicationManager.o" );

    while(1)
    {
	sleep(1);
    }	

    return 0; 
} 

void sig_handler( int signo)
{
    int i;
    int rc;
    int status;

    printf("ctrl-c\n");

    exit(1);
}
