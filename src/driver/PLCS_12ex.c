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

#include "../include/iDCU.h"
#include "../include/sqlite3.h"
#include "../include/M2MManager.h"
#include "../include/configRW.h"
#include "../include/writeLog.h"
#include "../include/TCPSocket.h"
#include "../include/libpointparser.h"
#include "../include/stringTrim.h"

static int plcs_12ex_id;
static int indexCount = 1;
static int xmlOffset = 0;
static NODEINFO xmlinfo;

static int selectTag_12ex(unsigned char* buffer, int trCount, int len );
static int ParsingReceiveValue_12ex(unsigned char* cvalue, int len, unsigned char* remainder, int remainSize );
static int Socket_Manager_12ex( int *client_sock );
static void *thread_main(void *arg);
static int createETRIPacketFormat( UINT8 *id, unsigned char parsing[1024][32], UINT16 parsingCnt );

//void *PLCS12ex(void *arg) {
void *PLCS12ex( DEVICEINFO *device) {

    int tcp;
    int i;
    int thr_id;
    int status;

    int rc;

    int initFail = 1;

    void *socket_fd;
    //int socket_fd;
    struct sockaddr_in servaddr; //server addr

    pthread_t p_thread;
    char th_data[256];

    xmlinfo = pointparser("/work/smart/tag_info.xml");
    /***************** MSG Queue **********************/
    if( -1 == ( plcs_12ex_id = msgget( (key_t)1, IPC_CREAT | 0666)))
    {
	    writeLog( "/work/smart/log", "[PLCS_10] error msgget() plcs_12ex_id" );
	    //perror( "msgget() ½ÇÆÐ");
	    return;
    }

    for( i = 0; i < xmlinfo.getPointSize; i++ )
    {
	//if (strcmp(xmlinfo.tag[i].driver,"PLCS12ex") == 0) 
	if (strcmp(xmlinfo.tag[i].id,device->id ) == 0) 
	{
	    printf("ID %s\n", xmlinfo.tag[i].id);
	    printf("IP %s\n", xmlinfo.tag[i].ip);
	    printf("PORT %s\n", xmlinfo.tag[i].port);
	    initFail = 0;
	    xmlOffset = i;
	}
    }
    
    if( initFail == 1 )
    return; 


    while(1)
    {
	//tcp = TCPServer( 50006 );
	//tcp = TCPClient( "10.1.0.6", "50006");
	tcp = TCPClient( xmlinfo.tag[xmlOffset].ip, atoi( xmlinfo.tag[xmlOffset].port ) );
	printf("Connect %d\n", tcp);
	//if( tcp != -1 )
	if( tcp > 0 )
	{
	    /***************** send thread **********************/
	    memcpy( th_data, (void *)&tcp, sizeof(tcp) );
	    if( pthread_create(&p_thread, NULL, &thread_main, (void *)th_data) == -1 )
	    {
		printf("error thread\n");
		writeLog( "/work/smart/log", "[PLCS12ex] error pthread_create" );
	    }


	    Socket_Manager_12ex( &tcp ); 
	}
	else
	{
	    close(tcp);
	}

	sleep(2);
	printf("========================================\n");
    }
    /*******************************************************/
    printf("End\n");

    return; 
} 

static void *thread_main(void *arg)
{

    int rtrn;
    int Fd;
    int length;
    unsigned char SendBuf[10] = { 0x22, 0x3a, 0x22, 0x2c, 0x33, 0x35, 0x2c, 0x63, 0x30, 0x0d };

    memcpy( &Fd, (char *)arg, sizeof(int));


    while(1)
    {
	rtrn = send( Fd, SendBuf, 10, MSG_NOSIGNAL);
	if( rtrn == -1 )
	    break;

	usleep(1000*atoi(xmlinfo.tag[xmlOffset].basescanrate));	// 500ms
    }
    printf("Exit Thread\n");
    writeLog( "/work/smart/log", "[PLCS12ex] Exit Thread" );

}
static int Socket_Manager_12ex( int *client_sock ) {

	fd_set control_msg_readset;
	struct timeval control_msg_tv;
	unsigned char DataBuf[BUFFER_SIZE*10];
	int ReadMsgSize;

	unsigned char receiveBuffer[BUFFER_SIZE*10];
	int receiveSize = 0;
	unsigned char remainder[BUFFER_SIZE*10];
	int parsingSize = 0;


	int rtrn;
	int i;
	int nd;


	FD_ZERO(&control_msg_readset);
	printf("Receive Ready!!!\n");
	writeLog( "/work/smart/comm/log/PLCS12ex", "[PLCS12ex] start");

	while( 1 ) 
	{

		FD_SET(*client_sock, &control_msg_readset);
		control_msg_tv.tv_sec = 55;
		//control_msg_tv.tv_usec = 10000;
		control_msg_tv.tv_usec = 5000000;	// timeout check 5 second

		nd = select( *client_sock+1, &control_msg_readset, NULL, NULL, &control_msg_tv );		
		if( nd > 0 ) 
		{

			memset( DataBuf, 0, sizeof(DataBuf) );
			ReadMsgSize = recv( *client_sock, &DataBuf, BUFFER_SIZE*10, MSG_DONTWAIT);
			if( ReadMsgSize > 0 ) 
			{
				/*
				for( i = 0; i < ReadMsgSize; i++ ) {
					//printf("%-3.2x", DataBuf[i]);
					printf("%C", DataBuf[i]);
				}
				printf("\n");

				*/
				printf("recv data size : %d\n", ReadMsgSize);
				printf("%s\n", DataBuf);
				writeLog( "/work/smart/comm/log/PLCS12ex", DataBuf);


				if( ReadMsgSize >= BUFFER_SIZE*10 )
				{
				    printf("wow continue\n");
					continue;
				}

				memcpy( receiveBuffer+receiveSize, DataBuf, ReadMsgSize );
				receiveSize += ReadMsgSize;

				if( receiveSize >= BUFFER_SIZE*10 )
				{
				    printf("Packet Buffer Full~~ %d\n", receiveSize );
				    writeLog( "/work/smart/comm/log/PLCS12ex", "[PLCS12ex] Packet Buffer Full~~");

				    receiveSize = 0;
				    memset( receiveBuffer, 0 , sizeof(BUFFER_SIZE)*10 );
				    memset( remainder, 0 , sizeof(BUFFER_SIZE)*10 );
				    continue;
				}


				parsingSize = ParsingReceiveValue_12ex(receiveBuffer, receiveSize, remainder, parsingSize);
				printf("reminder size %d \n", parsingSize );
				memset( receiveBuffer, 0 , sizeof(BUFFER_SIZE)*10 );
				receiveSize = 0;
				memcpy( receiveBuffer, remainder, parsingSize );
				receiveSize = parsingSize;

				memset( remainder, 0 , sizeof(BUFFER_SIZE)*10 );

			} 
			else {
				sleep(1);
				printf("receive None\n");
				writeLog( "/work/smart/comm/log/PLCS12ex", "[PLCS12ex] receive None");
				break;
			}
			ReadMsgSize = 0;

		} 
		else if( nd == 0 ) 
		{
			printf("timeout\n");
			writeLog( "/work/smart/comm/log/PLCS12ex", "[PLCS12ex] timeout");
			//shutdown( *client_sock, SHUT_WR );
			break;
		}
		else if( nd == -1 ) 
		{
			printf("error...................\n");
			writeLog( "/work/smart/comm/log/PLCS12ex", "[PLCS12ex] network error............");
			//shutdown( *client_sock, SHUT_WR );
			break;
		}
		nd = 0;

	}	// end of while

	printf("Disconnection client....\n");
	writeLog( "/work/smart/comm/log/PLCS12ex", "[PLCS12ex] Disconnection.....");

	close( *client_sock );
	return 0;

}

static int ParsingReceiveValue_12ex(unsigned char* cvalue, int len, unsigned char* remainder, int remainSize )
{

    unsigned char setBuffer[BUFFER_SIZE*10];
    int i;
    char* token ;

    UINT8 parsing[1024][32];
    UINT8 parsingCnt = 1;
    int crOffset = 0;


    for( i = 0; i < len; i++ )
    {

	if(  cvalue[i] == '"' && cvalue[i+1] == ':' && cvalue[i+2] == '"' && cvalue[i+3] == ',' && cvalue[i+4] == '3'  )
	{

	    for( crOffset = i; crOffset < len; crOffset++ )
	    {
		if( cvalue[crOffset] == 0x0d )
		{

		    memset( setBuffer, 0, BUFFER_SIZE*10 );
		    memset( parsing, 0, 1024*32 );
		    parsingCnt = 1;
		    
		    //stringOffset = 0;

		    memcpy( setBuffer, cvalue+i, crOffset-i );

		    token = strtok( setBuffer, ",");
		    strcpy( parsing[parsingCnt++], token );

		    while( token = strtok( NULL, "," ) ) {
			strcpy( parsing[parsingCnt++], token );
			//printf("[%d] %s\n", parsingCnt-1, parsing[parsingCnt-1] ) ;
		    }

		    createETRIPacketFormat( parsing[2], parsing, parsingCnt );


		    i = crOffset;
		    remainSize = i+1;
		    //printf(" i is %d, total Length %d\n", i, len );

		    crOffset = len;

		    //trCount = 1;

		}
		else
		{

		    if( crOffset+1 == len )
		    {

			printf("no CR~~~\n");
			remainSize = i;
			i += crOffset;
		    }
		}

	    }	//for( crOffset = i; crOffset < len )

	}
	else
	{

	    if( i+1 == len )
	    {
		//printf("packet clear!\n");
		i = len-1;
		remainSize = i+1;
	    }
	    else
	    {
		//printf(" i = %d, cvalue[%d] = %C\n", i, i, cvalue[i]);
	    }
	}

    }


    remainSize = len - remainSize;
    //printf("In~~~ remainSize %d\n", remainSize);
    memcpy( remainder, cvalue+(len-(remainSize)), remainSize );


    return remainSize;
}

static int createETRIPacketFormat( UINT8 *id, unsigned char parsing[1024][32], UINT16 parsingCnt )
{

    int stringOffset = 0;
    int idOffset = 0;
    UINT8 setString[BUFFER_SIZE*10];

    UINT8 trimBuffer[32];
    char* trimPoint;
    int trCount = 1;

    memset( setString, 0, BUFFER_SIZE*10 );

    if( strcmp(id , "36" ) == 0 )
    {
	//printf("36 OK\n");
	sprintf(setString+stringOffset, "%03d:%d;", 100, indexCount++ );
	stringOffset += strlen( setString );

	//printf("parsingCnt %d\n", parsingCnt );
	for( idOffset = 10; idOffset < parsingCnt; idOffset++ )
	{

	    {
		memset( trimBuffer, 0, 32);
		memcpy( trimBuffer, parsing+idOffset, 32 );
		//printf("%s\n", trimBuffer);
		trimPoint = trim(trimBuffer);
		sprintf(setString+stringOffset, "%03d:%s;", 101-10+idOffset, trimPoint );

		stringOffset += 5 + strlen(trimPoint);

		trCount++;
	    }
	}


	printf("%s\n", setString);

	selectTag_12ex( setString, trCount, strlen(setString) );

    }
    else if( strcmp(id, "33" ) == 0 )
    {

	// add index value hard coding	2014.11.26
	//printf("Index %d\n", indexCount);
	sprintf(setString+stringOffset, "%03d:%d;", 100, indexCount++ );
	stringOffset += strlen( setString );

	//printf("parsingCnt %d\n", parsingCnt );
	for( idOffset = 10; idOffset < 78; idOffset++ )
	{

	    if( idOffset-10 != 52 )
	    {
		memset( trimBuffer, 0, 32);
		memcpy( trimBuffer, parsing+idOffset, 32 );
		trimPoint = trim(trimBuffer);
		sprintf(setString+stringOffset, "%03d:%s;", 101-10+idOffset, trimPoint );

		stringOffset += 5 + strlen(trimPoint);

		trCount++;
	    }
	}


	writeLog( "/work/smart/comm/log", setString);
	printf("%s\n", setString);

	selectTag_12ex( setString, trCount, strlen(setString) );
    }

    return 0;

}

static int selectTag_12ex(unsigned char* buffer, int trCount, int len )
{

    t_data data;
    int i;
    int offset = 0;
    int cntOffset = 0;
    char* token ;
    char* trid;
    UINT8 parsing[1024][32];
    UINT8 parsingCnt = 0;


    memset( &data, 0, sizeof(t_data) );
    time_t sensingTime;

    time(&sensingTime);

    data.data_type = 1;

    //data.data_buff[offset++] = 69;   // transducer count
    data.data_buff[offset++] = trCount;   // transducer count

    printf("selectTag size %d, trCount %d\n", len, trCount);
    token = strtok( buffer, ";");
    strcpy( parsing[parsingCnt++], token );
    //printf("[%d] %s\n", parsingCnt-1, parsing[parsingCnt-1] ) ;
    while( token = strtok( NULL, ";" ) ) 
    {
	strcpy( parsing[parsingCnt++], token );
	//printf("[%d] %s\n", parsingCnt-1, parsing[parsingCnt-1] ) ;
    }

    //for( i = 0; i < 69; i++ )
    for( i = 0; i < trCount; i++ )
    {
	trid = strtok( parsing[i], ":");
	//printf("[%d] TR:%d", i, atoi(trid)) ;
	data.data_buff[offset++] = atoi(trid);   // transducer id 
	data.data_buff[offset++] = 0;   // transducer id 
	memcpy( data.data_buff+offset, &sensingTime, 4 );	// UTC
	trid = strtok( NULL, ":" );
	offset += 4;	// UTC length
	data.data_buff[offset++] = strlen(trid)+1;   // data length 
	//printf(" Len:%d", strlen(trid));
	memcpy( data.data_buff+offset, trid, strlen(trid) );	// sensing value
	offset += strlen(trid);	// value length
	data.data_buff[offset++] = 0;   // null data 
	//printf(" Value:%s\n", trid ) ;
    }
    data.data_num = offset; 


    if ( -1 == msgsnd( plcs_12ex_id, &data, sizeof( t_data) - sizeof( long), IPC_NOWAIT))
    {
	//perror( "msgsnd() error ");
	//writeLog( "msgsnd() error : Queue full" );
	writeLog("/work/smart/comm/log/PLCS12ex", "[PLCS12ex] msgsnd() error : Queue full" );
	//sleep(1);
	//return -1;
    }
    printf("[Comm->TagServer] Send Length %d\n", offset );

    return 0;
}
