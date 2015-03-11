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
#include "../include/hiredis/hiredis.h"
#include "../include/ETRI.h"



static redisContext *c;
static int tagID;
static char* ctagID;


static int plcs_12ex_id;
static int indexCount = 1;
static int xmlOffset = 0;
static NODEINFO xmlinfo;

static int LPUSH_SensingData(char redisValue[2048], int len );
static void redisInitialize();
static int selectTag_12ex(unsigned char* buffer, int trCount, int len );
static int ParsingReceiveValue_12ex(unsigned char* cvalue, int len, unsigned char* remainder, int remainSize );
static int Socket_Manager_12ex( int *client_sock );
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
    /*
    if( -1 == ( plcs_12ex_id = msgget( (key_t)1, IPC_CREAT | 0666)))
    {
	    writeLog( "/work/smart/comm/log/PLCS12ex", "[PLCS12ex]  error msgget() plcs_12ex_id" );
	    return;
    }
    */
    redisInitialize();

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
	    Socket_Manager_12ex( &tcp ); 
	}
	else
	{
	    close(tcp);
	    writeLog( "/work/smart/comm/log/PLCS12ex", "[PLCS12ex] fail Connection");
	}

	sleep(10);
	printf("========================================\n");
    }
    /*******************************************************/
    printf("End\n");
    writeLog( "/work/smart/comm/log/PLCS12ex", "[PLCSex] fail Connection");

    return; 
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
	writeLog( "/work/smart/comm/log/PLCS12ex", "[Socket_Manager_12ex] Start");

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
				    writeLog( "/work/smart/comm/log/PLCS12ex", "[Socket_Manager_12ex] Packet Buffer Full~~");

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
				printf("Network Disconnect\n");
				writeLog( "/work/smart/comm/log/PLCS12ex", "[Socket_Manager_12ex] Network Disconnect");
				break;
			}
			ReadMsgSize = 0;

		} 
		else if( nd == 0 ) 
		{
			printf("Timeout\n");
			writeLog( "/work/smart/comm/log/PLCS12ex", "[Socket_Manager_12ex] Timeout");
			//shutdown( *client_sock, SHUT_WR );
			break;
		}
		else if( nd == -1 ) 
		{
			printf("error...................\n");
			writeLog( "/work/smart/comm/log/PLCS12ex", "[Socket_Manager_12ex] network error............");
			//shutdown( *client_sock, SHUT_WR );
			break;
		}
		nd = 0;

	}	// end of while

	printf("Disconnection client....\n");
	//writeLog( "/work/smart/comm/log/PLCS12ex", "[PLCS12ex] Disconnection.....");

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


	//writeLog( "/work/smart/comm/log", setString);
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

    char redisValue[2048];
    memset(redisValue, 0, 2048 );
    /* Set a key */
    for( i = 0; i < data.data_num; i++ )
	sprintf(redisValue+(i*2), "%02X", data.data_buff[i]);


    LPUSH_SensingData(redisValue, strlen(redisValue)/2);


    /*
    if ( -1 == msgsnd( plcs_12ex_id, &data, sizeof( t_data) - sizeof( long), IPC_NOWAIT))
    {
	//perror( "msgsnd() error ");
	//writeLog( "msgsnd() error : Queue full" );
	writeLog("/work/smart/comm/log/PLCS12ex", "[selectTag_12ex] msgsnd() error : Queue full" );
	//sleep(1);
	//return -1;
    }
    */
    printf("[Comm->TagServer] Send Length %d\n", offset );

    return 0;
}
static int LPUSH_SensingData(char redisValue[2048], int len )
{

    static redisReply *reply;
    Struct_SensingValueReport pac;
    READENV env;

    int status, offset;
    int i,j;
    int rtrn;

    time_t  transferTime, sensingTime;
    unsigned char savePac[1024];
    char strPac[1024*2];


    memset( &env, 0, sizeof( READENV ) );

    ReadEnvConfig("/work/smart/reg/env.reg", &env );


    pac.Message_Id		= 0x09;
    pac.Length		= (SENSING_VALUE_REPORT_HEAD_SIZE-4) + len;
    pac.Command_Id		= 0xFFFFFFFF;
    pac.GateNode_Id		= env.gatenode;
    pac.PAN_Id			= env.pan;
    pac.SensorNode_Id		= env.sensornode; 

    time(&transferTime);
    pac.Transfer_Time		= transferTime;

    offset = 0;
    memset( savePac, 0, sizeof( savePac ) );

    memcpy( savePac+offset, &pac.Message_Id, sizeof( pac.Message_Id ) );
    offset += sizeof( pac.Message_Id );
    memcpy( savePac+offset, &pac.Length, sizeof( pac.Length ) );
    offset += sizeof( pac.Length );
    memcpy( savePac+offset, &pac.Command_Id, sizeof( pac.Command_Id ) );
    offset += sizeof( pac.Command_Id );
    memcpy( savePac+offset, &pac.GateNode_Id, sizeof( pac.GateNode_Id ) );
    offset += sizeof( pac.GateNode_Id );
    memcpy( savePac+offset, &pac.PAN_Id, sizeof( pac.PAN_Id ) );
    offset += sizeof( pac.PAN_Id );
    memcpy( savePac+offset, &pac.SensorNode_Id, sizeof( pac.SensorNode_Id ) );
    offset += sizeof( pac.SensorNode_Id );
    memcpy( savePac+offset, &pac.Transfer_Time, sizeof( pac.Transfer_Time ) );
    offset += sizeof( pac.Transfer_Time );

    memset( strPac, 0, 1024*2 );

    for( i = 0; i < offset; i++ ) 
	sprintf(strPac+(i*2), "%02X", savePac[i]);

    sprintf(strPac+strlen( strPac ), "%s", redisValue);



    reply = redisCommand(c,"lpush tag1 %s",   strPac);
    printf("type:%d / index:%lld\n", reply->type, reply->integer);
    printf("len:%d / str:%s\n", reply->len, reply->str);
    printf("elements:%d\n", reply->elements );


    writeLogV2( "/work/smart/comm/log/PLCS12ex", "LPUSH", "%s", strPac);

    if( reply->len > 0 )
	writeLogV2( "/work/smart/comm/log/PLCS12ex", "LPUSH", "%s", reply->str);
    rtrn = reply->integer;

    freeReplyObject(reply);


    return rtrn;

}
static void redisInitialize() {

    const char *hostname = "127.0.0.1";
    int port = 6379;
    struct timeval timeout = { 1, 500000 }; // 1.5 seconds

    c = redisConnectWithTimeout(hostname, port, timeout);

    if (c == NULL || c->err) {

	writeLog( "/work/smart/comm/log/PLCS12ex", "[redisInitialize] DB Fail");

	if (c) {
	    printf("Connection error: %s\n", c->errstr);
	    redisFree(c);
	} else {
	    printf("Connection error: can't allocate redis context\n");
	}
	exit(1);
    }
    else
	writeLog( "/work/smart/comm/log/PLCS12ex", "[redisInitialize] DB OK");


}


