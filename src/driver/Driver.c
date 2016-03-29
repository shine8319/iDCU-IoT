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

#include "../include/iDCU.h"
#include "../include/libpointparser.h"
#include "../include/libDeviceInfoParser.h"
#include "../include/hiredis/hiredis.h"

#include "./include/PLCS_9.h"
#include "./include/PLCS_10.h"
#include "./include/PLCS_12.h"
#include "./include/PLCS_12ex.h"
#include "./include/iDCU_IoT.h"
#include "./include/GN1200.h"
#include "./include/TK4M.h"

#define LOGPATH "/work/log/Driver"

static void sig_handler( int signo);
static void createDriver( DEVICEINFO *device, TAGINFO *tag );
static void createProcess( pid_t *pid, DEVICEINFO *device, TAGINFO *tag );

static void createDriver( DEVICEINFO *device, TAGINFO *tag )
{

    writeLogV2(LOGPATH, "[Start]", "[%s][%d] Open", device->driver, (int)getpid() );
    if (strcmp(device->driver,"PLCS9") == 0) 
    {
	PLCS9(device);
    }
    else if (strcmp(device->driver,"PLCS10") == 0) 
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
    else if (strcmp(device->driver,"GN1200") == 0) 
    {
	GN1200(device);
    }
    else
	printf("no exist~~~~~~\n");

}


int main(int argc, char **argv) { 

    int i;
    int threadCount = 0;
    int status;
    NODEINFO xmlinfo;
    pid_t pidCheck;
    //xmlinfo = pointparser("/work/smart/device_info.xml");
    xmlinfo = deviceInfoParser("/work/smart/device_info.xml");

    signal( SIGINT, (void *)sig_handler);

    pid_t pids[xmlinfo.getPointSize];
    printf("xmlinfo.getPointSize %d\n", xmlinfo.getPointSize);

    for( i = 0; i < xmlinfo.getPointSize; i++ )
    {
	createProcess( &pids[i], &xmlinfo.device[i], &xmlinfo.tag[i] );
	usleep(100000);	// 100ms
    }

    writeLog("/work/smart/log", "[Driver] start" );

    while(1)
    {

	for( i = 0; i < xmlinfo.getPointSize; i++ )
	{
	    pidCheck = waitpid( pids[i], &status, WNOHANG );
	    if( 0 != pidCheck )
	    {

		printf("[%s][%d] exit\n", xmlinfo.device[i].driver, pids[i] );
		writeLogV2(LOGPATH, "[Error]", "[%s][%d] exit", xmlinfo.device[i].driver, pids[i] );

		createProcess( &pids[i], &xmlinfo.device[i], &xmlinfo.tag[i] );
	    }

	    pidCheck = 0;
	    status = 0;
	    usleep(100000);	// 100ms
	}
	sleep(60);
    }	

    return 0; 
} 

static void createProcess( pid_t *pid, DEVICEINFO *device, TAGINFO *tag )
{

    *pid = fork();
    printf("[%s] *pid %d\n", device->driver, *pid);

    if( *pid < 0 )
    {
	perror("fork error : ");
	writeLogV2(LOGPATH, "[Error]", "[%s] error fork()", device->driver );
    }

    if( *pid == 0 )
    {
	printf("My pid %d\n", (int)getpid() );
	createDriver( device, tag );
	exit(0);
    }
    else
    {
	printf("Parent %d, child %d\n", (int)getpid(), *pid );
    }

}

static void sig_handler( int signo)
{
    int i;
    int rc;
    int status;

    printf("[%d] return Code %d\n", (int)getpid(), signo);
    writeLogV2(LOGPATH, "[Error]", "[%d] return Code %d", (int)getpid(), signo );

    exit(1);
}
