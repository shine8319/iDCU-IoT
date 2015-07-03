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

#include "./include/iDCU.h"

#include "./driver/include/PLCS_9.h"
#include "./driver/include/PLCS_10.h"
#include "./driver/include/PLCS_12.h"
#include "./driver/include/PLCS_12ex.h"
#include "./driver/include/iDCU_IoT.h"
#include "./driver/include/GN1200.h"

#include "./include/parser/serialInfoParser.h"
#include "./include/parser/configInfoParser.h"

#include "./include/uart.h"
#include "./include/debugPrintf.h"
#include "./include/libpointparser.h"
#include "./include/libDeviceInfoParser.h"

#include "./include/hiredis/hiredis.h"

#define BUFFER_SIZE 4096 

static SERIALINFO xmlinfo;
static CONFIGINFO configInfo;
static int xmlOffset = 0;

static int fd;
static int tcp;

static void init();
static void sig_handler( int signo);
static void createDriver( DEVICEINFO *device, TAGINFO *tag );
static void createProcess( pid_t *pid, DEVICEINFO *device, TAGINFO *tag );

static int startSerialToEthernetClient();
static int startSerialToEthernet();
static int modeConnect();
static void *thread_main(void *arg);

static void createDriver( DEVICEINFO *device, TAGINFO *tag )
{

    writeLogV2("/work/smart/comm/log/deviceLog", "[Start]", "[%s][%d] Open", device->driver, (int)getpid() );
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
    int rc;

    int status;
    pid_t pidCheck;
    signal( SIGINT, (void *)sig_handler);
    //xmlinfo = pointparser("/work/smart/device_info.xml");
    /*
       xmlinfo = deviceInfoParser("/work/smart/device_info.xml");


       pid_t pids[xmlinfo.getPointSize];
       printf("xmlinfo.getPointSize %d\n", xmlinfo.getPointSize);

       for( i = 0; i < xmlinfo.getPointSize; i++ )
       {
       createProcess( &pids[i], &xmlinfo.device[i], &xmlinfo.tag[i] );
       usleep(100000);	// 100ms
       }
     */
    init();

    fd = OpenSerial(xmlinfo.comport.baud, xmlinfo.comport.parity[0], xmlinfo.comport.databit[0], xmlinfo.comport.stopbit[0]);
    debugPrintf(configInfo.debug, "OpenSerial %d\n", fd);
    //rc = tty_raw(fd, xmlinfo.comport.baud, xmlinfo.comport.flowcontrol[0], xmlinfo.comport.databit[0], xmlinfo.comport.parity[0], xmlinfo.comport.stopbit[0]);
    //printf("tty_raw %d\n", rc);

    pthread_t p_thread;

    //writeLog("/work/smart/log", "[CommunicationManager] start CommunicationManager.o" );

    char th_data[256];
    UINT8 logBuffer[256];

    memset( th_data, 0, sizeof( th_data ) );
    debugPrintf(configInfo.debug, "%s mode....\n", xmlinfo.connect.mode);
 
    while(1)
    {

	tcp = modeConnect();

	if( tcp> 0 ) {
	    memcpy( th_data, (void *)&fd, sizeof(fd) );
	    if( pthread_create(&p_thread, NULL, &thread_main, (void *)th_data) == -1 )
	    //if( pthread_create(&p_thread, NULL, &thread_main, NULL ) == -1 )
	    {
		//writeLog("/work/iot/log", "[Uart] threads[2] thread_main error" );
		debugPrintf(configInfo.debug, "error thread\n");
	    }
	    else
	    {
		if( strcmp(xmlinfo.connect.mode,"SERVER") == 0 ) {
		    startSerialToEthernet();
		}
		else { 
		    startSerialToEthernetClient();
		}
	 

		close( tcp );
		tcp = -1;

		debugPrintf(configInfo.debug, "pthread_join\n");
		rc = pthread_join( p_thread, (void **)&status);
		if( rc == 0 )
		{
		    debugPrintf(configInfo.debug, "Completed join with thread status= %d\n", status);

		    //memset( logBuffer, 0, sizeof( logBuffer ) );
		    //sprintf( logBuffer, "[GN1200] Completed join with thread status= %d", status);
		    //writeLog( "/work/smart/comm/log/GN1200", logBuffer );
		}
		else
		{
		    debugPrintf(configInfo.debug, "ERROR; return code from pthread_join() is %d\n", rc);
		    //memset( logBuffer, 0, sizeof( logBuffer ) );
		    //sprintf( logBuffer, "[GN1200] ERROR; return code from pthread_join() is %d", rc);
		    //writeLog( "/work/smart/comm/log/GN1200", logBuffer );
		}

	    }
	}	
	else{
	    close(tcp);
	    tcp = -1;
	    //writeLog( "/work/smart/comm/log/GN1200", "[GN1200] fail Connection");
	}
        usleep(100000);	// 100ms
	sleep(2);

	/*
	   for( i = 0; i < xmlinfo.getPointSize; i++ )
	   {
	   pidCheck = waitpid( pids[i], &status, WNOHANG );
	   if( 0 != pidCheck )
	   {

	   printf("[%s][%d] exit\n", xmlinfo.device[i].driver, pids[i] );
	   writeLogV2("/work/smart/comm/log/deviceLog", "[Error]", "[%s][%d] exit", xmlinfo.device[i].driver, pids[i] );

	   createProcess( &pids[i], &xmlinfo.device[i], &xmlinfo.tag[i] );
	   }

	   pidCheck = 0;
	   status = 0;
	   usleep(100000);	// 100ms
	   }
	   sleep(60);
	 */
    }	

    return 0; 
} 
static void *thread_main(void *arg)
{

    int rtrn;

    int serial;

    int length;
    unsigned char SendBuf[BUFFER_SIZE];
    memset(SendBuf, 0, BUFFER_SIZE);

    memcpy( (void *)&serial, (char *)arg, sizeof(int));


    unsigned char serial_tx_buffer[4096];
    unsigned char serial_rx_buffer[4096];
    int rx_length;
    int tx_length;
    int i;


    while(1)
    {
	// serial to ethernet
	rx_length = read(serial, (void*)serial_rx_buffer, sizeof(serial_rx_buffer));	    //Filestream, buffer to store in, number of bytes to read (max)
	if (rx_length < 0)
	{
	    //An error occured (will occur if there are no bytes)
	    //printf("An error occured\n");
	    usleep(100000);	// 100ms
	    //break;
	}
	else if (rx_length == 0)
	{
	    //No data waiting
	    //printf("No data\n");
	    usleep(100000);	// 100ms
	}
	else
	{
	    //Bytes received, send to ethernet
	    serial_rx_buffer[rx_length] = '\0';
	    printf("%i bytes read : %s\n", rx_length, serial_rx_buffer);

	    if( tcp != -1 ) {

		//tx_length = send( tcp, serial_rx_buffer, rx_length, MSG_DONTWAIT);
		tx_length = send( tcp, serial_rx_buffer, rx_length, MSG_NOSIGNAL);
		printf("tx_length %d byte\n", tx_length );
		if( tx_length < 0 )
		{
		    debugPrintf(configInfo.debug,"tcp send error\n");

		    break;
		}
	    }
	    else
		break;
	}

	if( tcp == -1 )
	{
	    debugPrintf(configInfo.debug,"[thread] tcp error\n");
	    break;
	}


    }

    close( tcp );
    tcp = -1;

    debugPrintf(configInfo.debug, "Exit Thread\n");
    //writeLog( "/work/smart/comm/log/GN1200", "[thread_main] Exit Thread");
    pthread_exit((void *) 0);
}
static int startSerialToEthernetClient()
{
    unsigned char socket_tx_buffer[4096];
    unsigned char socket_rx_buffer[4096];
    unsigned char serial_tx_buffer[4096];
    unsigned char serial_rx_buffer[4096];
    int rx_length;
    int tx_length;

    fd_set control_msg_readset;
    struct timeval control_msg_tv;

    int rtrn;
    int i;
    int nd;

    suseconds_t timeout;
    timeout = 1000*atoi(xmlinfo.connect.timeout);
    debugPrintf(configInfo.debug, "timeout %ld mSec\n", timeout/1000 );

    FD_ZERO(&control_msg_readset);
 

    while(1) {

	FD_SET(tcp, &control_msg_readset);
	if( timeout <= 0)	// no timeout
	{
	    debugPrintf(configInfo.debug, "no timeout" );
	    nd = select( tcp+1, &control_msg_readset, NULL, NULL, NULL );		
	}
	else
	{
	    control_msg_tv.tv_usec = timeout;
	    nd = select( tcp+1, &control_msg_readset, NULL, NULL, &control_msg_tv );		
	}

	if( nd > 0 ) 
	{


	    rx_length = recv( tcp, &socket_rx_buffer, sizeof(socket_rx_buffer), MSG_DONTWAIT);

	    if( rx_length > 0 ) 
	    {

		tx_length = write(fd, &socket_rx_buffer, rx_length);	
		printf("tx_length %d byte\n", tx_length );
	    } 
	    else {
		//writeLog( "/work/smart/comm/log/GN1200", "[Socket_Manager] Network Disconnect");
		usleep(100000);	// 100ms
		debugPrintf(configInfo.debug, "network Disconnect\n");

		break;
	    }

	    if( tcp == -1 )
	    {
		debugPrintf(configInfo.debug, "client mode tcp disconnect\n");
		break;
	    }
	} 
	else if( nd == 0 ) 
	{
	    //writeLog( "/work/smart/comm/log/GN1200", "[Socket_Manager] Timeout");
	    debugPrintf(configInfo.debug, "timeout\n");
	    break;
	}
	else if( nd == -1 ) 
	{
	    //writeLog( "/work/smart/comm/log/GN1200", "[Socket_Manager] Network Error");
	    debugPrintf(configInfo.debug, "error.....\n");
	    break;
	}

	usleep(100000);	// 100ms


	//FD_SET(tcp, &control_msg_readset);
	//control_msg_tv.tv_sec = 55;
	//control_msg_tv.tv_usec = 5000000;	// timeout check 5 second

	//control_msg_tv.tv_usec = timeout;

	//usleep(100000);	// 100ms
	//usleep(1000*atoi(xmlinfo.tag[xmlOffset].basescanrate));	// 500ms

	// ethernet to serial
	//nd = select( tcp+1, &control_msg_readset, NULL, NULL, &control_msg_tv );		
	//printf("%d\n", nd);
	/*
	if( nd > 0 ) 
	{

	    rx_length = recv( tcp, &socket_rx_buffer, sizeof(socket_rx_buffer), MSG_DONTWAIT);

	    if( rx_length > 0 ) 
	    {

		tx_length = write(fd, &socket_rx_buffer, rx_length);	
		printf("tx_length %d byte\n", tx_length );
	    } 
	    else {
		//writeLog( "/work/smart/comm/log/GN1200", "[Socket_Manager] Network Disconnect");
		printf("network Disconnect\n");
		break;
	    }

	} 
	else if( nd == 0 ) 
	{
	    //writeLog( "/work/smart/comm/log/GN1200", "[Socket_Manager] Timeout");
	    debugPrintf(configInfo.debug, "timeout\n");
	    break;
	}
	else if( nd == -1 ) 
	{
	    //writeLog( "/work/smart/comm/log/GN1200", "[Socket_Manager] Network Error");
	    debugPrintf(configInfo.debug, "error.....\n");
	    break;
	}

	usleep(100000);	// 100ms
	*/
    }
    return 0;
}


static int startSerialToEthernet()
{
    unsigned char socket_tx_buffer[4096];
    unsigned char socket_rx_buffer[4096];
    unsigned char serial_tx_buffer[4096];
    unsigned char serial_rx_buffer[4096];
    int rx_length;
    int tx_length;

    fd_set control_msg_readset;
    struct timeval control_msg_tv;

    int rtrn;
    int i;
    int nd;

    suseconds_t timeout;
    timeout = 1000*atoi(xmlinfo.connect.timeout);
    debugPrintf(configInfo.debug, "timeout %ld mSec\n", timeout/1000 );

    FD_ZERO(&control_msg_readset);
 

    while(1) {

	FD_SET(tcp, &control_msg_readset);
	//control_msg_tv.tv_sec = 55;
	//control_msg_tv.tv_usec = 5000000;	// timeout check 5 second

	if( timeout <= 0)	// no timeout
	{
	    debugPrintf(configInfo.debug, "no timeout" );
	    nd = select( tcp+1, &control_msg_readset, NULL, NULL, NULL );		
	}
	else
	{
	    control_msg_tv.tv_usec = timeout;
	    nd = select( tcp+1, &control_msg_readset, NULL, NULL, &control_msg_tv );		
	}

	//usleep(100000);	// 100ms
	//usleep(1000*atoi(xmlinfo.tag[xmlOffset].basescanrate));	// 500ms

	// ethernet to serial
	//printf("%d\n", nd);
	if( nd > 0 ) 
	{

	    rx_length = recv( tcp, &socket_rx_buffer, sizeof(socket_rx_buffer), MSG_DONTWAIT);

	    if( rx_length > 0 ) 
	    {

		tx_length = write(fd, &socket_rx_buffer, rx_length);	
		printf("tx_length %d byte\n", tx_length );
	    } 
	    else {
		//writeLog( "/work/smart/comm/log/GN1200", "[Socket_Manager] Network Disconnect");
		printf("network Disconnect\n");
		break;
	    }

	} 
	else if( nd == 0 ) 
	{
	    //writeLog( "/work/smart/comm/log/GN1200", "[Socket_Manager] Timeout");
	    debugPrintf(configInfo.debug, "timeout\n");
	    break;
	}
	else if( nd == -1 ) 
	{
	    //writeLog( "/work/smart/comm/log/GN1200", "[Socket_Manager] Network Error");
	    debugPrintf(configInfo.debug, "error.....\n");
	    break;
	}

	usleep(100000);	// 100ms
    }
    return 0;
}

static void init()
{
    configInfo = configInfoParser("/work/smart/config/config.xml");
    xmlinfo = serialInfoParser("/work/config/serial_info.xml");
}

static int modeConnect()
{

    /***************** connect *********************/
    //debugPrintf(configInfo.debug, "%s mode....\n", xmlinfo.connect.mode);
    if( strcmp(xmlinfo.connect.mode,"SERVER") == 0 ) {
	tcp= TCPServer( atoi( xmlinfo.connect.localport ) );
    }
    else if( strcmp(xmlinfo.connect.mode,"CLIENT") == 0 ) {
	tcp= TCPClient( xmlinfo.connect.ip, atoi( xmlinfo.connect.port ) );
    }
    else if( strcmp(xmlinfo.connect.mode,"UDP") == 0 ) {

	close( tcp );
	tcp = -1;
	return -1;
    }
    else {

	close( tcp );
	tcp = -1;
	return -1;
    }


    return tcp;

}

static void createProcess( pid_t *pid, DEVICEINFO *device, TAGINFO *tag )
{

    *pid = fork();
    printf("[%s] *pid %d\n", device->driver, *pid);

    if( *pid < 0 )
    {
	perror("fork error : ");
	writeLogV2("/work/smart/comm/log/deviceLog", "[Error]", "[%s] error fork()", device->driver );
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
    writeLogV2("/work/smart/comm/log/deviceLog", "[Error]", "[%d] return Code %d", (int)getpid(), signo );

    exit(1);
}
