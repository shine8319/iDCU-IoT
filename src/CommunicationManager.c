#include <stdio.h> 
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
#include "./driver/include/PLCS_10.h"
#include "./driver/include/PLCS_12.h"
#include "./driver/include/PLCS_12ex.h"
#include "./driver/include/iDCU_IoT.h"

#include "./include/iDCU.h"
#include "./include/libpointparser.h"


void sig_handler( int signo);
pthread_t threads[8];

int main(int argc, char **argv) { 

    int i;
    pid_t pid;
    int threadCount = 0;
    NODEINFO xmlinfo;
    xmlinfo = pointparser("/work/smart/device_info.xml");
    char th_data[256];

    signal( SIGINT, (void *)sig_handler);

    printf("xmlinfo.getPointSize %d\n", xmlinfo.getPointSize);
    for( i = 0; i < xmlinfo.getPointSize; i++ )
    {
	if (strcmp(xmlinfo.tag[i].driver,"PLCS10") == 0) 
	{
	    if( pthread_create(&threads[i], NULL, &PLCS10, (void *)th_data) == -1 )
	    {
		writeLog("/work/smart/comm/log/deviceLog", "[CommunicationManager] error PLCS10 pthread_create" );
		printf("error thread\n");
	    }
	    else
	    {
		threadCount++;
		printf("threadCount %d\n", threadCount );
	    }

	}
	else if (strcmp(xmlinfo.tag[i].driver,"PLCS12") == 0) 
	{
	    if( pthread_create(&threads[i], NULL, &PLCS12, (void *)th_data) == -1 )
	    {
		writeLog("/work/smart/comm/log/deviceLog", "[CommunicationManager] error PLCS12 pthread_create" );
		printf("error thread\n");
	    }
	    else
	    {
		threadCount++;
		printf("threadCount %d\n", threadCount );
	    }


	}
	else if (strcmp(xmlinfo.tag[i].driver,"PLCS12ex") == 0) 
	{
	    if( pthread_create(&threads[i], NULL, &PLCS12ex, (void *)th_data) == -1 )
	    {
		writeLog("/work/smart/comm/log/deviceLog", "[CommunicationManager] error PLCS12ex pthread_create" );
		printf("error thread\n");
	    }
	    else
	    {
		threadCount++;
		printf("threadCount %d\n", threadCount );
	    }


	}
	else if (strcmp(xmlinfo.tag[i].driver,"iDCU_IoT") == 0) 
	{
	    if( pthread_create(&threads[i], NULL, &iDCU_IoT, (void *)th_data) == -1 )
	    {
		writeLog("/work/smart/comm/log/deviceLog", "[CommunicationManager] error iDCU_IoT pthread_create" );
		printf("error thread\n");
	    }
	    else
	    {
		threadCount++;
		printf("threadCount %d\n", threadCount );
	    }


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

    for (i = 7; i >= 0; i--)
    {
	rc = pthread_cancel(threads[i]); // 강제종료
	if (rc == 0)
	{
	    // 자동종료
	    rc = pthread_join(threads[i], (void **)&status);
	    if (rc == 0)
	    {
		printf("Completed join with thread %d status= %d\n",i, status);
	    }
	    else
	    {
		printf("ERROR; return code from pthread_join() is %d, thread %d\n", rc, i);
	    }
	}
    }
    exit(1);
}

