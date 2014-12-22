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

static int Socket_Manager_iDCU_IoT( int *client_sock );
static int ParsingReceiveValue_iDCU_IoT(unsigned char* cvalue, int len, unsigned char* remainder, int remainSize );
static int selectTag_iDCU_IoT(unsigned char* buffer, int len, INT32 type );
static void *thread_main(void *arg);

static int comm_id;
static NODEINFO xmlinfo;
static int xmlOffset = 0;
static UINT8 woPastData;
static UINT8 cntPastData[4];
static UINT32 preQty = 0;


void *iDCU_IoT(DEVICEINFO *device) {

    int tcpiDCU_IoT;
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
    if( -1 == ( comm_id = msgget( (key_t)1, IPC_CREAT | 0666)))
    {
	writeLog( "/work/smart/log", "[iDCU_IoT] error msgget() comm_id" );
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
	tcpiDCU_IoT = TCPClient( xmlinfo.tag[xmlOffset].ip, atoi( xmlinfo.tag[xmlOffset].port ) );
	printf("Connect %d\n", tcpiDCU_IoT);
	//if( tcpiDCU_IoT != -1 )
	if( tcpiDCU_IoT > 0 )
	{
	    /***************** send thread **********************/
	    memcpy( th_data, (void *)&tcpiDCU_IoT, sizeof(tcpiDCU_IoT) );
	    if( pthread_create(&p_thread, NULL, &thread_main, (void *)th_data) == -1 )
	    {
		//writeLog("/work/iot/log", "[Uart] threads[2] thread_main error" );
		printf("error thread\n");
	    }

	    Socket_Manager_iDCU_IoT( &tcpiDCU_IoT ); 
	}
	else
	    close(tcpiDCU_IoT);


	sleep(2);
	printf("========================================\n");

    }


    printf("End\n");

    return 0; 
} 

static void *thread_main(void *arg)
{

    int rtrn;
    int Fd;
    int length;
    int sendType = 0;
    unsigned char SendBuf[2][12] ={ { 0x01, 0x02, 0x00, 0x00, 0x00, 0x06, 0x01, 0x02, 0x00, 0x10, 0x00, 0x08 },
	{ 0x01, 0x02, 0x00, 0x00, 0x00, 0x06, 0x01, 0x04, 0x00, 0x10, 0x00, 0x10 } };

    memcpy( &Fd, (char *)arg, sizeof(int));


    while(1)
    {
	//rtrn = write(Fd, SendBuf, 12); 
	rtrn = send( Fd, SendBuf[sendType], 12, MSG_NOSIGNAL);
	//printf("return Send %d\n", rtrn );
	if( rtrn == -1 )
	    break;

	sendType = !sendType;
	//printf("sendType = %d\n", sendType);
	//sleep(1);

	//usleep(500000);	// 500ms
	//printf("BaseScanRate %dms\n", atoi(xmlinfo.tag[xmlOffset].basescanrate));
	usleep(1000*atoi(xmlinfo.tag[xmlOffset].basescanrate));	// 500ms
    }
    printf("Exit Thread\n");

}
static int Socket_Manager_iDCU_IoT( int *client_sock ) {

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

    writeLog("/work/smart/comm/log/iDCU_IoT", "[iDCU_IoT] start " );

    while( 1 ) 
    {

	FD_SET(*client_sock, &control_msg_readset);
	control_msg_tv.tv_sec = 15;
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

		parsingSize = ParsingReceiveValue_iDCU_IoT(receiveBuffer, receiveSize, remainder, parsingSize);
		memset( receiveBuffer, 0 , sizeof(BUFFER_SIZE) );
		receiveSize = 0;
		memcpy( receiveBuffer, remainder, parsingSize );
		receiveSize = parsingSize;

		memset( remainder, 0 , sizeof(BUFFER_SIZE) );

	    } 
	    else {
		sleep(1);
		printf("receive None\n");
		writeLog("/work/smart/comm/log/iDCU_IoT", "[iDCU_IoT] receive None" );
		break;
	    }
	    ReadMsgSize = 0;

	} 
	else if( nd == 0 ) 
	{
	    printf("timeout\n");
	    writeLog("/work/smart/comm/log/iDCU_IoT", "[iDCU_IoT] timeout" );
	    //shutdown( *client_sock, SHUT_WR );
	    break;
	}
	else if( nd == -1 ) 
	{
	    printf("error...................\n");
	    writeLog("/work/smart/comm/log/iDCU_IoT", "[iDCU_IoT] network error...." );
	    //shutdown( *client_sock, SHUT_WR );
	    break;
	}
	nd = 0;

    }	// end of while

    printf("Disconnection client....\n");

    writeLog("/work/smart/comm/log/iDCU_IoT", "[iDCU_IoT] Disconnection " );

    close( *client_sock );
    return 0;

}


static int ParsingReceiveValue_iDCU_IoT(unsigned char* cvalue, int len, unsigned char* remainder, int remainSize )
{

    int i, tlen;
    INT32 type;
    int offset;
    UINT8 byteBuffer[64];
    /*
       for( offset = 0; offset < len; offset++ )
       printf("%02X ", cvalue[offset]);
       printf("\n");
     */


    for( i = 0; i < len; i++ )
    {
	if( cvalue[i] == 0x01 && cvalue[i+1] == 0x02 )
	{ 
	    tlen = cvalue[i + 5];
	    if( tlen != cvalue[i + 5 + 3] + 3 )
	    {
		remainSize = 0;
		return remainSize;
	    }

	    /*
	       for( offset = i; offset < tlen + 6; offset++ )
	       printf("%02X ", cvalue[offset]);
	       printf("\n");
	       */

	    memcpy( byteBuffer, cvalue+(i+9), tlen-3 );
	    type = cvalue[i+7];

	    selectTag_iDCU_IoT( byteBuffer, tlen-3, type );

	    i = i + tlen + 6;
	    remainSize = i;
	}

    }

    remainSize = len - remainSize;
    memcpy( remainder, cvalue+(len-(remainSize)), remainSize );


    return remainSize;
}

static int selectTag_iDCU_IoT(unsigned char* buffer, int len, INT32 type )
{

    t_data data;
    int i;
    int offset = 0;
    int cntOffset = 0;

    memset( &data, 0, sizeof(t_data) );
    time_t sensingTime;

    UINT32 count = 0;
    UINT32 qty = 0;
    UINT8 dataChanged = 0;
    char countString[32];
    memset( countString, 0, 32 );

    time(&sensingTime);
    switch( type )
    {
	case 2:
	    data.data_type = 1;
	    //data.data_buff[offset++] = (0x01 & buffer[0]);
	    data.data_buff[offset++] = 1;   // transducer count
	    data.data_buff[offset++] = 51;   // transducer id 
	    data.data_buff[offset++] = 0;   // transducer id 
	    memcpy( data.data_buff+offset, &sensingTime, 4 );	// UTC
	    offset += 4;
	    data.data_buff[offset++] = 2;   // data length 

	    if( (0x02 & buffer[0]) )
		data.data_buff[offset++] = 0x31;   // sensing value
	    else
		data.data_buff[offset++] = 0x30;   // sensing value
	    data.data_buff[offset++] = 0;   // sensing value (null)

	    if( woPastData != (0x02 & buffer[0]) )
	    {
		dataChanged = 1;
		if( (0x02 & buffer[0]) )
		    writeLog("/work/smart/comm/log/iDCU_IoT", "WO" );
		else
		    writeLog("/work/smart/comm/log/iDCU_IoT", "NW" );

	    }

	    woPastData = (0x02 & buffer[0]);

	    data.data_num = offset; 
	    break;
	case 4:

	    data.data_type = 1;
	    /*
	       for(i = 4; i < 4+4; i++)
	       data.data_buff[offset++] = buffer[i];
	     */

	    data.data_buff[offset++] = 2;   // transducer count

	    // sum qty
	    data.data_buff[offset++] = 52;   // transducer id 
	    data.data_buff[offset++] = 0;   // transducer id 
	    memcpy( data.data_buff+offset, &sensingTime, 4 );	// UTC
	    offset += 4;

	    count = (0x000000ff & (UINT32)buffer[1]) << 0;
	    count |= (0x000000ff & (UINT32)buffer[0]) << 8;
	    count |= (0x000000ff & (UINT32)buffer[3]) << 16;
	    count |= (0x000000ff & (UINT32)buffer[2]) << 24;

	    /*
	       printf("count %ld, %d\n", count, strlen(ltoa(count, data.data_buff+offset, 10)) );


	     */

	    sprintf( countString, "%ld", count );
	    data.data_buff[offset++] = strlen( countString )+1;   // data length 
	    //printf("len %02X \n", data.data_buff[offset-1]);

	    sprintf( data.data_buff+offset, "%ld", count );
	    offset += strlen( countString );

	    // end
	    data.data_buff[offset++] = 0;   // sensing value (null)

	    // qty
	    data.data_buff[offset++] = 53;   // transducer id 
	    data.data_buff[offset++] = 0;   // transducer id 
	    memcpy( data.data_buff+offset, &sensingTime, 4 );	// UTC
	    offset += 4;

	    if( preQty == 0 || preQty > count )
	    {
		cntPastData[1] = buffer[1];
		cntPastData[0] = buffer[0];
		cntPastData[3] = buffer[3];
		cntPastData[2] = buffer[2];

		preQty = (0x000000ff & (UINT32)cntPastData[1]) << 0;
		preQty |= (0x000000ff & (UINT32)cntPastData[0]) << 8;
		preQty |= (0x000000ff & (UINT32)cntPastData[3]) << 16;
		preQty |= (0x000000ff & (UINT32)cntPastData[2]) << 24;

		break;
	    }

	    qty = count - preQty;


	   //printf("count %ld, %d\n", count, strlen(ltoa(count, data.data_buff+offset, 10)) );

	    //printf("qty = count - preQty (%ld = %ld - %ld)\n", qty, count, preQty ); 

	    memset( countString, 0, sizeof( countString ));
	    sprintf( countString, "%ld", qty);
	    data.data_buff[offset++] = strlen( countString )+1;   // data length 
	    //printf("len %02X \n", data.data_buff[offset-1]);

	    sprintf( data.data_buff+offset, "%ld", qty);
	    offset += strlen( countString );

	    // end
	    data.data_buff[offset++] = 0;   // sensing value (null)



	    if( cntPastData[1] != buffer[1] ||
		    cntPastData[0] != buffer[0] ||
		    cntPastData[3] != buffer[3] ||
		    cntPastData[2] != buffer[2]    )
	    {
		dataChanged = 1;

		writeLog("/work/smart/comm/log/iDCU_IoT", countString  );
	    }

	    cntPastData[1] = buffer[1];
	    cntPastData[0] = buffer[0];
	    cntPastData[3] = buffer[3];
	    cntPastData[2] = buffer[2];

	    preQty = (0x000000ff & (UINT32)cntPastData[1]) << 0;
	    preQty |= (0x000000ff & (UINT32)cntPastData[0]) << 8;
	    preQty |= (0x000000ff & (UINT32)cntPastData[3]) << 16;
	    preQty |= (0x000000ff & (UINT32)cntPastData[2]) << 24;


	    data.data_num = offset; 
	    break;


    }


    if( dataChanged )
    {
	for( i=0; i<data.data_num; i++ )
	    printf("%02X ", data.data_buff[i]);
	printf("\n");



	if ( -1 == msgsnd( comm_id, &data, sizeof( t_data) - sizeof( long), IPC_NOWAIT))
	{
	    //perror( "msgsnd() error ");
	    //writeLog( "msgsnd() error : Queue full" );

	    writeLog("/work/smart/comm/log/iDCU_IoT", "[iDCU_IoT] msgsnd() error : Queue full" );
	    //sleep(5);
	    //return -1;
	}
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
