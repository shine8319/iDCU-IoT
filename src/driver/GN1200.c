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

#define POLYNORMIAL 0xA001

static UINT16 CRC16(UINT8 *puchMsg, int usDataLen);
static void *thread_main(void *arg);
static int Socket_Manager( int *client_sock );
static int ParsingReceiveValue(unsigned char* cvalue, int len, unsigned char* remainder, int remainSize );
static int selectTag(unsigned char* buffer, int len, INT32 type );

static int comm_id;
static NODEINFO xmlinfo;
static int xmlOffset = 0;

void *GN1200(DEVICEINFO *device) {

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

    UINT8 logBuffer[256];


    xmlinfo = pointparser("/work/smart/tag_info.xml");

    /***************** MSG Queue **********************/
    if( -1 == ( comm_id = msgget( (key_t)1, IPC_CREAT | 0666)))
    {
	writeLog( "/work/smart/comm/log/GN1200", "[GN1200] error msgget() comm_id" );
	//perror( "msgget() ½ÇÆÐ");
	return;
    }

    for( i = 0; i < xmlinfo.getPointSize; i++ )
    {
	//if (strcmp(xmlinfo.tag[i].driver,"iDCU_IoT") == 0) 
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


	/***************** connect *********************/

	printf("Try Connect....\n");
	tcp= TCPClient( xmlinfo.tag[xmlOffset].ip, atoi( xmlinfo.tag[xmlOffset].port ) );
	printf("Connect %d\n", tcp);
	//if( tcp!= -1 )
	if( tcp> 0 )
	{
	    /***************** send thread **********************/
	    memcpy( th_data, (void *)&tcp, sizeof(tcp) );
	    if( pthread_create(&p_thread, NULL, &thread_main, (void *)th_data) == -1 )
	    {
		writeLog( "/work/smart/comm/log/GN1200", "[GN1200] error pthread_create");
		printf("error thread\n");
	    }
	    else
	    {
		writeLog( "/work/smart/comm/log/GN1200", "[GN1200] Start Thread");
		Socket_Manager( &tcp); 
		printf("pthread_join\n");

		rc = pthread_join( p_thread, (void **)&status);
		if( rc == 0 )
		{
		    printf("Completed join with thread status= %d\n", status);

		    memset( logBuffer, 0, sizeof( logBuffer ) );
		    sprintf( logBuffer, "[GN1200] Completed join with thread status= %d", status);
		    writeLog( "/work/smart/comm/log/GN1200", logBuffer );
		}
		else
		{
		    printf("ERROR; return code from pthread_join() is %d\n", rc);
		    memset( logBuffer, 0, sizeof( logBuffer ) );
		    sprintf( logBuffer, "[GN1200] ERROR; return code from pthread_join() is %d", rc);
		    writeLog( "/work/smart/comm/log/GN1200", logBuffer );

		    //return -1;
		}
	    }

	}
	else
	{
	    close(tcp);
	    writeLog( "/work/smart/comm/log/GN1200", "[GN1200] fail Connection");
	}


	sleep(10);
	printf("========================================\n");

    }


    printf("End\n");
    writeLog( "/work/smart/comm/log/GN1200", "[GN1200] End main");

    return 0; 
} 

static UINT16 CRC16(UINT8 *puchMsg, int usDataLen){
    int i;
    UINT16 crc, flag;
    crc = 0xffff;
    while(usDataLen--){
	crc ^= *puchMsg++;
	for (i=0; i<8; i++){
	    flag = crc & 0x0001;
	    crc >>= 1;
	    if(flag) crc ^= POLYNORMIAL;
	}
    }
    return crc;
}

static void *thread_main(void *arg)
{

    int rtrn;
    int Fd;
    int length;
    int sendType = 0;

    UINT16 crc16;

    unsigned char SendBuf[8] ={ 0x01, 0x03, 0x00, 0x00, 0x00, 0x04, 0x44, 0x09 };

    memcpy( &Fd, (char *)arg, sizeof(int));


    while(1)
    {
	//rtrn = send( Fd, SendBuf, 8, MSG_NOSIGNAL);
	rtrn = send( Fd, SendBuf, 8, MSG_DONTWAIT); //2015.03.02 
	//printf("return Send %d\n", rtrn );
	if( rtrn == -1 )
	{
	    printf("[Thread] Error Send\n");
	    break;
	}


	//usleep(500000);	// 500ms
	//printf("BaseScanRate %dms\n", atoi(xmlinfo.tag[xmlOffset].basescanrate));
	usleep(1000*atoi(xmlinfo.tag[xmlOffset].basescanrate));	// 500ms
    }
    printf("Exit Thread\n");
    writeLog( "/work/smart/comm/log/GN1200", "[thread_main] Exit Thread");

    pthread_exit((void *) 0);

}
static int Socket_Manager( int *client_sock ) {

    fd_set control_msg_readset;
    struct timeval control_msg_tv;
    unsigned char DataBuf[BUFFER_SIZE];
    int ReadMsgSize;

    unsigned char receiveBuffer[BUFFER_SIZE];
    int receiveSize = 0;
    unsigned char remainder[BUFFER_SIZE];
    int parsingSize = 0;


    int rtrn;
    int i;
    int nd;

    FD_ZERO(&control_msg_readset);
    printf("Receive Ready!!!\n");
    writeLog( "/work/smart/comm/log/GN1200", "[Socket_Manager] Start");

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
	    ReadMsgSize = recv( *client_sock, &DataBuf, BUFFER_SIZE, MSG_DONTWAIT);
	    if( ReadMsgSize > 0 ) 
	    {
		/*
		   for( i = 0; i < ReadMsgSize; i++ ) {
		   printf("%-3.2x", DataBuf[i]);
		   }
		   printf("\n");
		   printf("recv data size : %d\n", ReadMsgSize);
		 */

		if( ReadMsgSize >= BUFFER_SIZE )
		    continue;

		memcpy( receiveBuffer+receiveSize, DataBuf, ReadMsgSize );
		receiveSize += ReadMsgSize;

		parsingSize = ParsingReceiveValue(receiveBuffer, receiveSize, remainder, parsingSize);
		memset( receiveBuffer, 0 , sizeof(BUFFER_SIZE) );
		receiveSize = 0;
		memcpy( receiveBuffer, remainder, parsingSize );
		receiveSize = parsingSize;

		memset( remainder, 0 , sizeof(BUFFER_SIZE) );

	    } 
	    else {
		sleep(1);
		writeLog( "/work/smart/comm/log/GN1200", "[Socket_Manager] Network Disconnect");
		printf("network Disconnect\n");
		break;
	    }
	    ReadMsgSize = 0;

	} 
	else if( nd == 0 ) 
	{
	    writeLog( "/work/smart/comm/log/GN1200", "[Socket_Manager] Timeout");
	    printf("timeout\n");
	    break;
	}
	else if( nd == -1 ) 
	{
	    writeLog( "/work/smart/comm/log/GN1200", "[Socket_Manager] Network Error");
	    printf("error...................\n");
	    break;
	}
	nd = 0;

    }	// end of while

    printf("Disconnection client....\n");

    close( *client_sock );
    return 0;

}


static int ParsingReceiveValue(unsigned char* cvalue, int len, unsigned char* remainder, int remainSize )
{

    int i, tlen;
    INT32 type;
    int offset;
    UINT8 byteBuffer[64];
    UINT16 crc16;
    UINT8 u8Crc16[0];
    /*
       for( offset = 0; offset < len; offset++ )
       printf("%02X ", cvalue[offset]);
       printf("\n");
     */


    for( i = 0; i < len; i++ )
    {
	if( cvalue[i] == 0x01 && cvalue[i+1] == 0x03 && cvalue[i+2] == 0x08 )
	{ 
	    tlen = cvalue[i + 2];
	    /*
	       if( tlen != cvalue[i + 5 + 3] + 3 )
	       {
	       remainSize = 0;
	       return remainSize;
	       }
	     */
	    crc16 = CRC16(cvalue+i, 3+tlen);
	    u8Crc16[0] = (UINT8)((crc16>>0) & 0x00ff);
	    u8Crc16[1] = (UINT8)((crc16>>8) & 0x00ff);
	    if( cvalue[i+3+8+0] == u8Crc16[0] &&
		    cvalue[i+3+8+1] == u8Crc16[1] )
	    {
		//printf("crc OK\n");

		for( offset = i; offset < tlen + 5; offset++ )
		    printf("%02X ", cvalue[offset]);
		printf("\n");

		memcpy( byteBuffer, cvalue+(i+5), 2 );

		selectTag( byteBuffer, 2, 0 );

		i = i + tlen + 5;
		remainSize = i;

	    }
	    else {

		i = len-1;
		remainSize = i+1;
	    }

	}
	else
	{

	    if( i+1 == len )
	    {
		//printf("packet clear!\n");
		writeLog( "/work/smart/comm/log/GN1200", "[ParsingReceiveValue] packet clear!");
		i = len-1;
		remainSize = i+1;
	    }
	    else
	    {
		// i + +
	    }
	}



    }

    remainSize = len - remainSize;
    memcpy( remainder, cvalue+(len-(remainSize)), remainSize );


    return remainSize;
}

static int selectTag(unsigned char* buffer, int len, INT32 type )
{

    t_data data;
    int i;
    int offset = 0;
    int cntOffset = 0;

    UINT16 temperature;

    for( i = 0; i < len; i++ ) {
	printf("%02X ", buffer[i]);

    }
    printf("\n");

    memset( &data, 0, sizeof(t_data) );

    time_t sensingTime;
    char countString[32];

    memset( countString, 0, 32 );

    time(&sensingTime);

    data.data_type = 1;

    data.data_buff[offset++] = 1;   // transducer count

    data.data_buff[offset++] = 61;   // transducer id 
    data.data_buff[offset++] = 0;   // transducer id 
    memcpy( data.data_buff+offset, &sensingTime, 4 );	// UTC
    offset += 4;

    temperature = (UINT16)((buffer[0]<<8) & 0xff00);
    temperature |= (UINT16)((buffer[1]<<0) & 0x00ff);

    printf("temperature %.2f\n", temperature/10.0);

    sprintf( countString, "%.2f", temperature/10.0 );
    data.data_buff[offset++] = strlen( countString )+1;   // data length 
    //printf("len %02X \n", data.data_buff[offset-1]);
    writeLog("/work/smart/comm/log/GN1200", countString );

    sprintf( data.data_buff+offset, "%.2f", temperature/10.0 );
    offset += strlen( countString );
    data.data_buff[offset++] = 0;   // sensing value (null)

    data.data_num = offset; 


    /*
       printf("event => ");
       for( i=0; i<data.data_num; i++ )
       printf("%02X ", data.data_buff[i]);
       printf("\n");
     */

    if ( -1 == msgsnd( comm_id, &data, sizeof( t_data) - sizeof( long), IPC_NOWAIT))
    {
	writeLog("/work/smart/comm/log/GN1200", "[selectTag] msgsnd() error : Queue full" );
    }

    return 0;
}

/*
   unsigned char _getCheckSum_iDCU_IoT( int len )
   {
   int intSum = 0;
   unsigned char sum;
   int i;

//printf("start sum %d\n", len);
for( i = 1; i<= len; i++ )
{
intSum += tlvSendBuffer[i];
//sleep(1);
//printf("%d ", tlvSendBuffer[i]);
}

sum = (unsigned char)intSum;
//printf("sum %d\n", sum);
return sum;
}

 */
