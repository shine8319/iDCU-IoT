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

#include "./include/iDCU.h"
#include "./include/sqlite3.h"
//#include "./include/SQLite3Interface.h"
#include "./include/writeLog.h"
#include "./include/TCPSocket.h"
#include "./include/configRW.h"
#include "./include/ETRI.h"
#include "./include/ETRI_Registration.h"
#include "./include/ETRI_Deregistration.h"

#define BUFFER_SIZE 1024

int tcp;
pthread_t threads[2];
int pushStart;
READENV env;

void *thread_sender(void *arg);
void *thread_receive(void *arg);
Struct_SensingValueReport pac;

int main( void)
{
    int      msqid;
    int      eventid;
    int 		i,j;
    int 		id = 0;
    int			rtrn;
    char		cntOffset = 9;
    t_data   data;
    t_data   eventData;
    time_t  transferTime, sensingTime;

    char *query;
    int	getPortOffset = 0;
    //int	dataChanged = 0;
    int	packetErr = 0;
    unsigned char checkSumCompare = 0;
    UINT8 sendBuffer[1024];
    //clock_t start,end;
    //double msec;

    //thread
    char th_data[256];
    char th2_data[256];

    memset( &env, 0, sizeof( READENV ) );
    memset( &data, 0, sizeof(t_data) );
    memset( &eventData, 0, sizeof(t_data) );
    memset( sendBuffer, 0, 1024 );



    int status, rc;
    struct sockaddr_in ip; 
    ReadEnvConfig("/work/smart/reg/env.reg", &env );

    ip.sin_addr.s_addr = (env.middleware_ip);
    printf(" bind IP %s, port %d\n", inet_ntoa(ip.sin_addr), env.middleware_port);

    int reTry = 3;


    do {

	tcp = TCPClient( inet_ntoa(ip.sin_addr), env.middleware_port);
	if( tcp == -1 )
	{
	    writeLog("/work/smart/log", "[tagServer] fail TCPClient method " );
	    printf("connect error\n");
	    return -1;
	}
	else
	    printf("connect success\n");



	if( -1 == ETRI_Registration( &tcp, &env ) )
	{
	    //printf("fail registration\n");
	    writeLog( "/work/smart/log", "[tagServer] fail registration" );
	    //return -1;
	    sleep(1);
	    reTry--;
	    if( reTry == 0 )
		return -1;
	}
	else
	    reTry = 0;

    } while( reTry );

    /*
    memcpy( th_data, (void *)&tcp, sizeof(tcp) );
    if( pthread_create(&threads[0], NULL, &thread_receive, (void *)th_data) == -1 )
    {
	writeLog("./log", "[tagServer] error pthread_create method" );
	//printf("error thread\n");
    }
    */
    /*
       if( pthread_create(&threads[1], NULL, &thread_sender, (void *)th_data) == -1 )
       {
       writeLog("./log", "[pthread_create] thread_sender error" );
       printf("error thread\n");
       }
     */



    if ( -1 == ( msqid = msgget( (key_t)1, IPC_CREAT | 0666)))
    {
	writeLog( "/work/smart/log", "[tagServer] error msgget() msqid" );
	return -1;
    }

    int ReadMsgSize;
    unsigned char DataBuf[1024];
    unsigned char sendBuf[1024];

    writeLog( "/work/smart/log", "[tagServer] Start tagServer.o" );

    while( 1 )
    {

	ReadMsgSize = recv( tcp, &DataBuf, BUFFER_SIZE, MSG_DONTWAIT);
	if( ReadMsgSize > 0 ) 
	{
	    for( i = 0; i < ReadMsgSize; i++ ) {
		printf("%-3.2x", DataBuf[i]);
	    }
	    printf("\n");
	    printf("[ETRI return] recv data size : %d\n", ReadMsgSize);
	}

	
	memset( &data, 0, sizeof(t_data) );
	if( -1 == msgrcv( msqid, &data, sizeof( t_data) - sizeof( long), 0, IPC_NOWAIT) )
	{
	    //printf("Empty Q\n");
	    sleep(1);
	}
	else
	{
	    printf("[MSG] recv size = %d, type = %ld\n", data.data_num, data.data_type);
	    for( i = 0; i < data.data_num; i++ )
		printf("%02X ", data.data_buff[i]);
	    printf("\n");


	    //printf("Push Start %d\n", pushStart );

	    int bufferOffset = 0;

	    if( data.data_type == 1 )
	    {
		pac.Message_Id		= 0x09;
		pac.Length		= (SENSING_VALUE_REPORT_HEAD_SIZE-4) + data.data_num;
		pac.Command_Id		= 0xFFFFFFFF;
		pac.GateNode_Id		= env.gatenode;
		pac.PAN_Id			= env.pan;
		pac.SensorNode_Id		= env.sensornode; 
		time(&transferTime);
		pac.Transfer_Time		= transferTime;
		memcpy( pac.Transducer_Info, data.data_buff, data.data_num );
		//pac.Transducer_Count	= 2;
		rtrn = send(tcp, &pac, SENSING_VALUE_REPORT_HEAD_SIZE + data.data_num, MSG_NOSIGNAL);
		//rtrn = send(tcp, &pac, SENSING_VALUE_REPORT_HEAD_SIZE + data.data_num, MSG_DONTWAIT);

		if( rtrn == -1 )
		{
		    close(tcp);
		    sleep(1);
		    tcp = TCPClient( inet_ntoa(ip.sin_addr), env.middleware_port);
		    if( tcp == -1 )
		    {
			writeLog("/work/smart/log", "[tagServer] error TCPClient method " );
		    }
		    /*
		    else
		    {

			// 자동종료
			rc = pthread_join(threads[0], (void **)&status);
			if (rc == 0)
			{
			    printf("Completed join with thread status= %d\n", status);
			}
			else
			{
			    printf("ERROR; return code from pthread_join() is %d\n", rc);
			    //return -1;
			}
			memcpy( th_data, (void *)&tcp, sizeof(tcp) );
			if( pthread_create(&threads[0], NULL, &thread_receive, (void *)th_data) == -1 )
			{
			    writeLog("./log", "[tagServer] error pthread_create method" );
			    //printf("error thread\n");
			}
		    }
		    */

		}
		else
		{
		    printf("send length = %d\n", rtrn);
		    memset( sendBuf, 0, 1024);
		    memcpy( sendBuf, &pac, rtrn );
		    for( i = 0; i < rtrn; i++ )
			printf("%02X ", sendBuf[i]);
		    printf("\n");

		}


	    }

	}

    }

}
void *thread_sender(void *arg)
{

    int rtrn;
    int Fd;
    int length;
    unsigned char SendBuf[BUFFER_SIZE];
    memset(SendBuf, 0, BUFFER_SIZE);

    memcpy( (void *)&Fd, (char *)arg, sizeof(int));


    while(1)
    {
	rtrn = send(Fd, &pac, sizeof(Struct_SensingValueReport) ,MSG_NOSIGNAL);
	printf("send length = %d\n", rtrn);
	if( rtrn == -1 )
	{
	    close(Fd);
	    break;
	    //tcp = TCPClient( inet_ntoa(ip.sin_addr), env.middleware_port);
	}

	//usleep(500000);	// 500ms
	usleep(1000000);	// 
    }

}

void *thread_receive(void *arg)
{

    int Fd;
    int length;
    unsigned char SendBuf[1024];
    fd_set control_msg_readset;
    struct timeval control_msg_tv;
    unsigned char DataBuf[1024];
    int ReadMsgSize;

    unsigned char receiveBuffer[1024];
    int receiveSize = 0;
    unsigned char remainder[1024];
    int parsingSize = 0;

    int rtrn = 0;
    int i;
    int nd;
    memset(SendBuf, 0, 1024);

    memcpy( (void *)&Fd, (char *)arg, sizeof(int));

    writeLog("/work/smart/log", "[tagServer] Start thread_receive" );
    printf("thread start polling time %ldsec\n", env.timeout);

    FD_ZERO(&control_msg_readset);
    //printf("Receive Ready!!!\n");

    while( 1 ) 
    {

	FD_SET(Fd, &control_msg_readset);
	control_msg_tv.tv_sec = env.timeout;
	//control_msg_tv.tv_usec = 10000;
	control_msg_tv.tv_usec = 0;	// timeout check 5 second

	nd = select( Fd+1, &control_msg_readset, NULL, NULL, &control_msg_tv );		
	if( nd > 0 ) 
	{

	    memset( DataBuf, 0, sizeof(DataBuf) );
	    ReadMsgSize = recv( Fd, &DataBuf, BUFFER_SIZE, MSG_DONTWAIT);
	    if( ReadMsgSize > 0 ) 
	    {
		for( i = 0; i < ReadMsgSize; i++ ) {
		    printf("%-3.2x", DataBuf[i]);
		}
		printf("\n");
		printf("recv data size : %d\n", ReadMsgSize);

		if( ReadMsgSize >= BUFFER_SIZE )
		    continue;

		if( DataBuf[0] == 0x13 )
		{
		    printf("User Defined Message\n");

		    if( -1 == ETRI_Deregistration( &Fd, &env ) )
		    {
			printf("fail Deregistration\n");
			//writeLog( "./log", "fail registration" );
			//return -1;
		    }


		}

		/*
		   memcpy( receiveBuffer+receiveSize, DataBuf, ReadMsgSize );
		   receiveSize += ReadMsgSize;

		   parsingSize = ParsingReceiveValue(receiveBuffer, receiveSize, remainder, parsingSize);
		   memset( receiveBuffer, 0 , sizeof(BUFFER_SIZE) );
		   receiveSize = 0;
		   memcpy( receiveBuffer, remainder, parsingSize );
		   receiveSize = parsingSize;

		   memset( remainder, 0 , sizeof(BUFFER_SIZE) );
		 */

	    } 
	    else {
		sleep(1);
		printf("[tagServer] receive None %d\n", ReadMsgSize);
		//break;
	    }
	    ReadMsgSize = 0;

	} 
	else if( nd == 0 ) 
	{
	    printf("[tagServer] timeout %ldsec\n", env.timeout );
	    //shutdown( *tcp, SHUT_WR );
	    pushStart = 1;
	    close(Fd);
	    break;
	}
	else if( nd == -1 ) 
	{
	    printf("[tagServer] error...................\n");
	    //shutdown( *tcp, SHUT_WR );
	    close(Fd);
	    break;
	}
	nd = 0;

    }	// end of while

    writeLog("/work/smart/log", "[tagServer] Exit thread_receive" );
    printf("thread Exit\n");

}
