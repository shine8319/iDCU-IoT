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

int plcs_12ex_id;
int indexCount = 1;

void *PLCS12ex(void *arg) {

    int tcp;
    int i;
    pthread_t p_thread;
    int thr_id;
    int status;

    int rc;

    int initFail = 1;
    int xmlOffset = 0;

    void *socket_fd;
    //int socket_fd;
    struct sockaddr_in servaddr; //server addr

    NODEINFO xmlinfo;
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
	if (strcmp(xmlinfo.tag[i].driver,"PLCS12ex") == 0) 
	{
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
	if( tcp != -1 )
	    Socket_Manager_12ex( &tcp ); 
	else
	    close(tcp);

	sleep(2);
	printf("========================================\n");
    }
    /*******************************************************/
    printf("End\n");

    return; 
} 

int Socket_Manager_12ex( int *client_sock ) {

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

	while( 1 ) 
	{

		FD_SET(*client_sock, &control_msg_readset);
		control_msg_tv.tv_sec = 55;
		//control_msg_tv.tv_usec = 10000;
		control_msg_tv.tv_usec = 5000000;	// timeout check 5 second

		// ¸®ÅÏ°ª -1Àº ¿À·ù¹ß»ý, 0Àº Å¸ÀÓ¾Æ¿ô, 0º¸´Ù Å©¸é º¯°æµÈ ÆÄÀÏ µð½ºÅ©¸³ÅÍ¼ö
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
				printf("%s\n", DataBuf);
				writeLog( "/work/smart/comm/log/PLCS12ex", DataBuf);

				printf("recv data size : %d\n", ReadMsgSize);

				if( ReadMsgSize >= BUFFER_SIZE*10 )
					continue;

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
				break;
			}
			ReadMsgSize = 0;

		} 
		else if( nd == 0 ) 
		{
			printf("timeout\n");
			//shutdown( *client_sock, SHUT_WR );
			break;
		}
		else if( nd == -1 ) 
		{
			printf("error...................\n");
			//shutdown( *client_sock, SHUT_WR );
			break;
		}
		nd = 0;

	}	// end of while

	printf("Disconnection client....\n");

	close( *client_sock );
	return 0;

}

int ParsingReceiveValue_12ex(unsigned char* cvalue, int len, unsigned char* remainder, int remainSize )
{

    unsigned char setBuffer[BUFFER_SIZE*10];
    int i;
    int stringOffset = 0;
    int idOffset = 0;
    char* token ;
    UINT8 parsing[256][256];
    UINT8 setString[BUFFER_SIZE*10];
    UINT8 parsingCnt = 1;
    UINT8 ngOffset = 32;


    for( i = 0; i < len; i++ )
    {

	if(  cvalue[i] == '"' && cvalue[i+1] == ':' && cvalue[i+2] == '"' && cvalue[i+3] == ',' && cvalue[i+4] == '3' && cvalue[i+5] == '3'  )
	{


	    if( len+remainSize < BUFFER_SIZE*10 && cvalue[len-1] == 0x0d )
	    {

		token = strtok( cvalue, ",");
		strcpy( parsing[parsingCnt++], token );
		//printf("[%d] %s\n", parsingCnt-1, parsing[parsingCnt-1] ) ;

		while( token = strtok( NULL, "," ) ) 
		{
		    
		    strcpy( parsing[parsingCnt++], token );
		    //printf("[%d] %s\n", parsingCnt-1, parsing[parsingCnt-1] ) ;
		}

	
		// add index value hard coding	2014.11.26
		printf("Index %d\n", indexCount);
		sprintf(setString+stringOffset, "%03d:%d;", 100, indexCount++ );
		stringOffset += strlen( setString );

		for( idOffset = i+10; idOffset < i+78; idOffset++ )
		{
		    sprintf(setString+stringOffset, "%03d:%s;", 101-(i+10)+idOffset, parsing[idOffset] );

		    stringOffset += 5 + strlen(parsing[idOffset]);

		}


		writeLog( "/work/smart/comm/log", setString);
		printf("%s\n", setString);
	
		i += len-1;
		remainSize = i+1;

		selectTag_12ex( setString, strlen(setString) );
	    }
	    else
	    {
		i += len-1;

		if( len+remainSize >= BUFFER_SIZE*10 )
		{
		    printf("packet Buffer Full %d\n", len+remainSize );
		    writeLog( "/work/smart/comm/log/PLCS12ex", "[PLCS12ex] Packet Buffer Full~~");
		    remainSize = i+1;
		}
		else
		    remainSize = 0;
	    }

	}
	else
	{

	    printf("packet clear!\n");
	    i += len-1;
	    remainSize = i+1;
	}

    }

    remainSize = len - remainSize;
    memcpy( remainder, cvalue+(len-(remainSize)), remainSize );


    return remainSize;
}

int selectTag_12ex(unsigned char* buffer, int len )
{

    t_data data;
    int i;
    int offset = 0;
    int cntOffset = 0;
    char* token ;
    char* trid;
    UINT8 parsing[256][256];
    UINT8 parsingCnt = 0;


    memset( &data, 0, sizeof(t_data) );
    time_t sensingTime;

    time(&sensingTime);

    data.data_type = 1;

    data.data_buff[offset++] = 69;   // transducer count

    printf("selectTag size %d\n", len);
    token = strtok( buffer, ";");
    strcpy( parsing[parsingCnt++], token );
    //printf("[%d] %s\n", parsingCnt-1, parsing[parsingCnt-1] ) ;
    while( token = strtok( NULL, ";" ) ) 
    {
	strcpy( parsing[parsingCnt++], token );
	//printf("[%d] %s\n", parsingCnt-1, parsing[parsingCnt-1] ) ;
    }

    for( i = 0; i < 69; i++ )
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
