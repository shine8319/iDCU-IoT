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
//#include <wiringPi.h>

#include "./include/wiringPi.h"
#include "./include/iDCU.h"
#include "./include/writeLog.h"
#include "./include/getDiffTime.h"
#include "./include/sqlite3.h"
#include "./include/sqliteManager.h"

#include "./include/debugPrintf.h"
#include "./include/parser/configInfoParser.h"

#include "./include/hiredis/hiredis.h"
#include "./include/hiredis/async.h"
#include "./include/hiredis/adaters/libevent.h"

#include "./include/RedisControl.h"

#define CONFIGINFOPATH "/work/config/config.xml"
#define LOGPATH "/work/log/ModbusRTU"
#define DBPATH "/work/db/comm"
#define SQLITE_SAFE_FREE(x)	if(x){ x = NULL; }
#define BUFFER_SIZE 2048
#define	BAUDRATE	115200	
#define DI_PORT		8
#define LED_PORT	2
#define BUTTON_PORT	3

enum {
    BLUE = 0,
    WHITE = 1
};

typedef struct 
{
    UINT8 cnt_inc_size;
    UINT8 toggle_size;
    UINT8 start_io_check;
    UINT8 predi;
    UINT8 curdi;
    UINT32 precnt;
    UINT32 cnt;
    UINT8 subcnt;
    UINT32 send;

} IOCheckStruct;
IOCheckStruct		IO[DI_PORT];

unsigned char _getCheckSum( int len );
unsigned char _uartGetCheckSum( unsigned char* buffer, int len );
void *thread_main(void *);
void *ConnectProcess();
void sig_handler( int signo);
int setCountClear(unsigned char* buffer, int len, UINT8 port, UINT8 value );
int setRedisEventCountClear( INT32 port );
int vGetDIStatus(void);

int setDO(UINT8* buffer, int len, UINT8 port, UINT8 value );
static void onMessage(redisAsyncContext *c, void *reply, void *privdata);

pthread_t threads[5];

StatusSendData statusSend[8];
StatusSendData countSend;
UINT8 tlvSendBuffer[BUFFER_SIZE];

MGData pastData;
MGData node;
UINT8 io[8];


sqlite3 *pSQLite3;


int server_sock; 
int client_sock; 
int fd; // Serial Port File Descriptor

char diPin[DI_PORT] = { 5,6,4,14,13, 12, 3, 2 };	// c1 pin 18,22,16,23
char ledPin[LED_PORT] = { 10, 11 };
char buttonPin[BUTTON_PORT] = { 7, 1, 0 };
char doPin[4] = { 13, 12, 3, 2 };
/*
char diPin[4] = { 5,6,4,14 };	// c1 pin 18,22,16,23
*/

static redisContext *event;
static CONFIGINFO configInfo;


int main(int argc, char **argv) { 

    int i, led;
    pthread_t p_thread;
    int thr_id;
    int status;

    int rc;

    void *socket_fd;
    struct sockaddr_in servaddr; //server addr

    // wifi
    struct termios tio, old_tio;
    int ret;

    unsigned char DataBuf[BUFFER_SIZE];
    int ReadMsgSize;

    unsigned char receiveBuffer[BUFFER_SIZE];
    int receiveSize = 0;
    unsigned char remainder[BUFFER_SIZE];
    int parsingSize = 0;

    char th_data[256];

    int dataToggle = 0;

    memset( &pastData, 0, sizeof( MGData ) );
    memset( &io, 0, sizeof( UINT8 )*8 );

    signal( SIGINT, (void *)sig_handler);

    wiringPiSetup () ;
    event = redisInitialize();

    /*
    for (i = 0 ; i < 6 ; ++i)
	pinMode (doPin[i], OUTPUT) ;
	*/

    // DI 8 port
    for (i = 0 ; i < DI_PORT ; i++)
	pinMode (diPin[i], INPUT) ;

    // LED 2 port
    for (i = 0 ; i < LED_PORT ; i++)
	pinMode (ledPin[i], OUTPUT) ;

    // BUTTON 3 port
    for (i = 0 ; i < BUTTON_PORT ; i++)
	pinMode (buttonPin[i], INPUT) ;


    // DB Open
    rc = _sqlite3_open( DBPATH, &pSQLite3 );
    if( rc != 0 )
    {
	writeLogV2(LOGPATH, "[ModBusRTU]", "error DB Open\n");
	return -1;
    }
    else
    {
	writeLogV2(LOGPATH, "[ModBusRTU]", "DB Open\n");
	//printf("%s OPEN!!\n", DBPATH);
    }


    //sqlite3_busy_timeout( pSQLite3, 1000);
    //sqlite3_busy_timeout( pSQLite3, 5000);

    // DB Customize
    rc = _sqlite3_customize( &pSQLite3 );
    if( rc != 0 )
    {
	writeLogV2(LOGPATH, "[ModBusRTU]", "error DB Customize\n");
	return -1;
    }
    rc = _sqlite3_nolock( &pSQLite3 );
    if( rc != 0 )
    {
	writeLogV2(LOGPATH, "[ModBusRTU]", "error set nolock\n");
	return -1;
    }


    configInfo = configInfoParser(CONFIGINFOPATH);

    rc = init();
    if( rc == -1 )
    {
	writeLogV2(LOGPATH, "[ModBusRTU]", "error init()\n");
	return -1;
    }

    for( i = 0; i < 8; i++)
    {
	IO[i].cnt_inc_size = check_interval( atoi(configInfo.port[i].interval) );
	//printf(" %d ", IO[i].cnt_inc_size);
    }
    //printf("\n");


    /***************** Server Connect **********************/
    if( pthread_create(&threads[2], NULL, &thread_main, NULL) == -1 )
    {
	writeLogV2(LOGPATH, "[ModBusRTU]", "thread_main create error\n");
	printf("error thread\n");
    }

    if( pthread_create(&threads[1], NULL, &ConnectProcess, NULL ) == -1 )
    {
	writeLogV2(LOGPATH, "[ModBusRTU]", "ConnectProcess create error\n");
    }
    usleep(1000);

    writeLogV2(LOGPATH, "[ModBusRTU]", "Start\n");
    while(1)
    { 
	delay (10) ;
	if( vGetDIStatus() )
	{

	    _sqlite3_update( &pSQLite3, node );
	    
	    printf("DI => ");
	    for( i = 0; i < DI_PORT; i++ )
	    {
		printf(" %ld | ", IO[i].cnt);
	    }
	    printf("\n");

	    printf("BUTTON => ");
	    for( i = 0; i < BUTTON_PORT; i++ )
	    {
		printf(" %d | ", !digitalRead (buttonPin[i]));
	    }
	    printf("\n");

	    digitalWrite(ledPin[WHITE], dataToggle);
	    printf("dataToggle %d\n", dataToggle);
	    if( dataToggle == 0 )
		dataToggle = 1;
	    else
		dataToggle = 0;


	    //digitalWrite(ledPin[0], !digitalRead (buttonPin[0])); //blue
	    //digitalWrite(ledPin[1], !digitalRead (buttonPin[1]));

	}

    }




    /*******************************************************/
    _sqlite3_close( &pSQLite3 );

    writeLogV2(LOGPATH, "[ModBusRTU]", "End\n");
    return 0; 
} 
static void onMessage(redisAsyncContext *c, void *reply, void *privdata) {

    int status, offset;
    int i,j;
    redisReply *r = reply;
    time_t  transferTime, sensingTime;
    unsigned char savePac[1024];


    if (reply == NULL) return;

    if (r->type == REDIS_REPLY_ARRAY) {
	for (j = 0; j < r->elements; j++) {
	    printf("%u) %s\n", j, r->element[j]->str);

	}

	if (strcasecmp(r->element[0]->str,"message") == 0 &&
		strcasecmp(r->element[1]->str,"clear") == 0) {

	    setRedisEventCountClear(atoi(r->element[2]->str));

	    redisReply *callbackReply;
	    //callbackReply = redisCommand(event,"PUBLISH callbackClearCommand 1:%d;2:%d;3:%d;4:%d;5:%d;6:%d;7:%d;8:%d;", 
	    callbackReply = redisCommand(event,"PUBLISH callbackClearCommand %d;%d;%d;%d;%d;%d;%d;%d;", 
					    IO[0].cnt,
					    IO[1].cnt,
					    IO[2].cnt,
					    IO[3].cnt,
					    IO[4].cnt,
					    IO[5].cnt,
					    IO[6].cnt,
					    IO[7].cnt );

	    printf("type:%d / integer:%lld\n", callbackReply->type, callbackReply->integer);
	    printf("len:%d / str:%s\n", callbackReply->len, callbackReply->str);
	    printf("elements:%d\n", callbackReply->elements );


	}
    }
}


void sig_handler( int signo)
{
    int i;
    int rc;
    int status;

    printf("ctrl-c\n");

    for (i = 4; i >= 0; i--)
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

int init( void ) {

    char *query;
    int i,j;
    int	rowSize = 8;
    unsigned int byteOrder = 0;

    SQLite3Data data;

    query = sqlite3_mprintf("select port1, port2, port3, port4, port5, port6, port7, port8 from tb_comm_status");	// edit 20140513


    data = _sqlite3_select( pSQLite3, query );


    if( data.size > 0 )
    {

	for( i = 0; i < (data.size-8)/8; i++ )
	{

	    for( j = 0; j < 8; j++ )
	    {
		pastData.cnt[j] = atoi(data.data[rowSize++]);
		node.cnt[j] = pastData.cnt[j];
		IO[j].cnt = pastData.cnt[j];

	    }

	}
    }
    else
	return -1;


    return 0;
}

int SocketProcess( void ) {

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

    while( 1 ) 
    {

	FD_SET(client_sock, &control_msg_readset);
	control_msg_tv.tv_sec = 15;
	//control_msg_tv.tv_usec = 10000;
	control_msg_tv.tv_usec = 5000000;	// timeout check 5 second

	// ¸®ÅÏ°ª -1Àº ¿À·ù¹ß»ý, 0Àº Å¸ÀÓ¾Æ¿ô, 0º¸´Ù Å©¸é º¯°æµÈ ÆÄÀÏ µð½ºÅ©¸³ÅÍ¼ö
	nd = select( client_sock+1, &control_msg_readset, NULL, NULL, &control_msg_tv );		
	if( nd > 0 ) 
	{

	    memset( DataBuf, 0, sizeof(DataBuf) );
	    ReadMsgSize = recv( client_sock, &DataBuf, BUFFER_SIZE, MSG_DONTWAIT);
	    if( ReadMsgSize > 0 ) 
	    {
		for( i = 0; i < ReadMsgSize; i++ ) {
		    printf("%-3.2x", DataBuf[i]);
		}
		printf("\n");
		printf("recv data size : %d\n", ReadMsgSize);

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
		printf("receive None\n");
		break;
	    }
	    ReadMsgSize = 0;

	} 
	else if( nd == 0 ) 
	{
	    printf("timeout\n");
	    //shutdown( client_sock, SHUT_WR );
	    break;
	}
	else if( nd == -1 ) 
	{
	    printf("error...................\n");
	    //shutdown( client_sock, SHUT_WR );
	    break;
	}
	nd = 0;

    }	// end of while

    printf("Disconnection client....\n");

    close( client_sock );
    return 0;

}


void *ConnectProcess() {

    struct sockaddr_in servaddr; //server addr
    struct sockaddr_in clientaddr; //client addr
    unsigned short port = 502;
    //unsigned short port = 9000;
    int client_addr_size;
    int bufsize = 100000;
    int rn = sizeof(int);
    int flags;
    pid_t pid;

    printf("[ModbusRTU] Start!!\n");
    writeLogV2(LOGPATH, "[ConnectProcess]", "Start\n");

    while( 1 ) {

	/* create socket */
	server_sock = socket(PF_INET, SOCK_STREAM, 0);

	// add by hyungoo.kang
	// solution ( bind error : Address already in use )
	if( setsockopt( server_sock, SOL_SOCKET, SO_REUSEADDR, (void*)&bufsize, (socklen_t)rn) < 0 ) {
	    perror("Error setsockopt()");
	    writeLogV2(LOGPATH, "[ConnectProcess]", "Error setsockopt()\n");
	    return;
	}

	memset(&servaddr, 0, sizeof(servaddr));
	servaddr.sin_family = AF_INET;
	servaddr.sin_addr.s_addr = htonl(INADDR_ANY);
	servaddr.sin_port = htons(port);

	if (bind(server_sock, (struct sockaddr*)&servaddr, sizeof(servaddr)) < 0) {
	    printf("server socket bind error\n");
	    writeLogV2(LOGPATH, "[ConnectProcess]", "server socket bind error\n");
	    return;
	}

	listen(server_sock, 5);

	client_addr_size = sizeof(clientaddr);
	client_sock = accept(server_sock, (struct sockaddr*)&clientaddr, &client_addr_size);


	if((flags = fcntl(client_sock, F_GETFL, 0)) == -1 ) {
	    perror("fcntl");
	    return;
	}
	if(fcntl(client_sock, F_GETFL, flags | O_NONBLOCK) == -1) {
	    perror("fcntl");
	    return;
	}

	close( server_sock );

        writeLogV2(LOGPATH, "[ConnectProcess]", "New Connection, Client IP : %s (%d)\n",	inet_ntoa(clientaddr.sin_addr), client_sock );
	printf("[ModbusRTU] New Connection, Client IP : %s (%d)\n",	inet_ntoa(clientaddr.sin_addr), client_sock);
	SocketProcess();
	printf("[ModbusRTU] TCP : Client No Signal. Connection Try Again....\n");
        writeLogV2(LOGPATH, "[ConnectProcess]", "TCP : Client No Signal. Connection Try Again....\n");

    }

    close( client_sock );

    writeLogV2(LOGPATH, "[ConnectProcess]", "End\n");
    pthread_exit((void *) 0);
}

int ParsingReceiveValue(unsigned char* cvalue, int len, unsigned char* remainder, int remainSize )
{

    unsigned char setBuffer[BUFFER_SIZE];
    int i;
    int size;
    int offset;

    for( i = 0; i < len; i++ )
    {

	if( cvalue[i] == 0x01 && cvalue[i+1] == 0x02 )
	{

	    size = cvalue[i+5]+6;

	    memcpy( setBuffer, cvalue+i, size );

	    /*
	       for( offset = 0; offset < size; offset++ )
	       printf("%02X ", setBuffer[offset]);
	       printf("\n");
	     */


	    i += size-1;

	    remainSize = i+1;

	    CommandProcess( setBuffer, size );


	}

    }

    remainSize = len - remainSize;
    memcpy( remainder, cvalue+(len-(remainSize)), remainSize );


    return remainSize;
}


int vGetDIStatus(void)
{
    UINT8 i;
    UINT8 changed = 0;

    for(i =0; i <DI_PORT ; i++)
    {

	IO[i].curdi =  !digitalRead (diPin[i]);

	if( IO[i].start_io_check )
	{
	    if(  (short)IO[i].predi == (short)IO[i].curdi )
	    {

		if( IO[i].subcnt > IO[i].cnt_inc_size )
		{

		    IO[i].start_io_check = 0;
		    IO[i].cnt++;
		    IO[i].send = IO[i].cnt;

		    node.cnt[i] = IO[i].cnt;
		    changed = 1;
		    //   _sqlite3_update( &pSQLite3, node );
		    //MGData node
		    //rpmemcpy( DataHIB.Port[i].Value, &IO[i].cnt, 4);

		    //sprintf( info.Count[i], "%01d:%06ld",  (short)(i+1), IO[i].cnt % 1000000 ); // issue perform

		    IO[i].subcnt = 0;

		}
		else
		{
		    IO[i].subcnt++;
		}
	    }
	    else
	    {
		IO[i].start_io_check = 0;
	    }
	}
	else
	{

	    if(  IO[i].predi > IO[i].curdi )
	    {
		IO[i].start_io_check = 1;
		IO[i].subcnt = 0;
	    }
	    else if(  IO[i].predi == IO[i].curdi )
		IO[i].subcnt = 0;

	}

	if( IO[i].cnt == 0xFFFFFFFF )
	{
	    IO[i].cnt = 0;
	    IO[i].send = 0;
	}

	IO[i].predi = IO[i].curdi;


    }

    return changed;


}

int receiveEvent( unsigned char* buffer, int len ) 
{
    int rc;
    int status;
    int cntOffset;
    int transaction = 0;
    UINT8 i;
    UINT8 changed = 0;

    cntOffset = 4;

    switch( buffer[2] )
    {
	case 1:
	    //CreateSync( buffer[4] );
	    break;
	default:
	    for( i = 0; i < 8; i++ )
	    {
		node.cnt[i] = 0;

		node.cnt[i] = (0x000000ff & (UINT32)buffer[cntOffset++]) << 24;
		node.cnt[i] |= (0x000000ff & (UINT32)buffer[cntOffset++]) << 16;
		node.cnt[i] |= (0x000000ff & (UINT32)buffer[cntOffset++]) << 8;
		node.cnt[i] |= (0x000000ff & (UINT32)buffer[cntOffset++]) << 0;

		cntOffset += 2;

		if( pastData.cnt[i] != node.cnt[i] )
		    changed = 1;

	    }
	    for( i = 0; i < 8; i++ )
	    {
		io[i] = 0;
		io[i] = buffer[cntOffset++];

		cntOffset += 2;
	    }

	    if( changed )
	    {
		transaction = _sqlite3_transaction( &pSQLite3, 1 ) ;
		if( transaction == 0 )
		{
		    _sqlite3_update( &pSQLite3, node );
		    do {
			transaction = _sqlite3_transaction( &pSQLite3, 2 ) ;
		    } while( transaction );

		}

		for( i = 0; i < 8; i++ )
		    printf("[%d] %ld/%ld ", i, pastData.cnt[i], node.cnt[i] );
		printf("\n");
	    }

	    for( i = 0; i < 8; i++ )
	    {
		//value[id].curWo[i] = node.wo[i];
		pastData.cnt[i] = node.cnt[i];
	    }

	    break;
    }

}

int CommandProcess( unsigned char* buffer, int len ) 
{
    int i;
    int rtrn;
    UINT8 type;
    UINT8 port;
    UINT8 value;

    type = buffer[7];

    switch( type )
    {
	case 1:
	    getDOStatus( );
	    //getIOStatus( );
	    break;
	case 2:
	    getIOStatus( );
	    break;
	case 4:
	    //getCommStatus( );
	    getCount( );
	    break;
	case 5:
	    port = buffer[9];
	    if( buffer[10] == 0xff )
		value = 0;
	    else
		value = 1;
	    if( port < 8 )
		setDO(buffer, len, port, value);
	    else
		setCountClear(buffer, len, port, value);
	    break;
    }

    return 0;

}

int setDO(UINT8* buffer, int len, UINT8 port, UINT8 value ) {

    int rtrn;
    unsigned char SendBuf[BUFFER_SIZE];
    memset(SendBuf, 0, BUFFER_SIZE);
    printf("port %d, value %d\n", port, value);
    digitalWrite (doPin[port], value);


    rtrn = write(client_sock, buffer, len );
    printf(" send %d~~~~~~~~~~~~~~\n", rtrn);
    return 0;
}

int setCountClear(unsigned char* buffer, int len, UINT8 port, UINT8 value ) {

    int rtrn, changed = 0;
    unsigned char SendBuf[BUFFER_SIZE];
    memset(SendBuf, 0, BUFFER_SIZE);
    printf("count clear port %d\n", port );

    switch (port){
	case 0x22:
	case 0x23:
	case 0x24:
	case 0x25:
	case 0x26:
	case 0x27:
	case 0x28:
	case 0x29:
	    IO[port-34].cnt = 0;
	    node.cnt[port-34] = 0;
	    changed = 1;
	    break;
    }

    if( changed )
	_sqlite3_update( &pSQLite3, node );

    rtrn = write(client_sock, buffer, len );
    printf(" send %d~~~~~~~~~~~~~~\n", rtrn);
    return 0;
}
int setRedisEventCountClear( INT32 port ) {

    int rtrn, changed = 0;
    printf("count clear port %ld\n", port );

      switch (port){
	case 0x22:
	case 0x23:
	case 0x24:
	case 0x25:
	case 0x26:
	case 0x27:
	case 0x28:
	case 0x29:
	    IO[port-34].cnt = 0;
	    node.cnt[port-34] = 0;
	    changed = 1;
	    break;
    }

    if( changed )
	_sqlite3_update( &pSQLite3, node );



    return 0;
}
int getDOStatus( ) {

    UINT8   SendBuffer[32];
    UINT8   i;

    SendBuffer[0] = 0x01;
    SendBuffer[1] = 0x02;
    SendBuffer[2] = 0x00;
    SendBuffer[3] = 0x00;
    SendBuffer[4] = 0x00;
    SendBuffer[5] = 0x04;    // t length
    SendBuffer[6] = 0x01;
    SendBuffer[7] = 0x01;
    SendBuffer[8] = 0x01;

    SendBuffer[9] = 0;

    //printf (" | %d\n", digitalRead (doPin[led])) ;
    SendBuffer[9] |= !digitalRead (doPin[0]) << 0;
    SendBuffer[9] |= !digitalRead (doPin[1]) << 1;
    SendBuffer[9] |= !digitalRead (doPin[2]) << 2;
    SendBuffer[9] |= !digitalRead (doPin[3]) << 3;
    /*
       SendBuffer[9] |= io[4] << 4;
       SendBuffer[9] |= io[5] << 5;
       SendBuffer[9] |= io[6] << 6;
       SendBuffer[9] |= io[7] << 7;
     */


    int rtrn = write(client_sock, SendBuffer, SendBuffer[5]+6 );
    /*
       for( i = 0; i < rtrn; i++ ) {
       printf("%-3.2x", SendBuffer[i]);
       }
       printf("\n");
     */

    return 0;
}


int getIOStatus( ) {

    UINT8   SendBuffer[32];
    UINT8   i;

    SendBuffer[0] = 0x01;
    SendBuffer[1] = 0x02;
    SendBuffer[2] = 0x00;
    SendBuffer[3] = 0x00;
    SendBuffer[4] = 0x00;
    SendBuffer[5] = 0x04;    // t length
    SendBuffer[6] = 0x01;
    SendBuffer[7] = 0x02;
    SendBuffer[8] = 0x01;

    SendBuffer[9] = 0;
    SendBuffer[9] |= !digitalRead (diPin[0]) << 0;
    SendBuffer[9] |= !digitalRead (diPin[1]) << 1;
    SendBuffer[9] |= !digitalRead (diPin[2]) << 2;
    SendBuffer[9] |= !digitalRead (diPin[3]) << 3;
    /*
       SendBuffer[9] |= io[4] << 4;
       SendBuffer[9] |= io[5] << 5;
       SendBuffer[9] |= io[6] << 6;
       SendBuffer[9] |= io[7] << 7;
     */


    int rtrn = write(client_sock, SendBuffer, SendBuffer[5]+6 );
    /*
       for( i = 0; i < rtrn; i++ ) {
       printf("%-3.2x", SendBuffer[i]);
       }
       printf("\n");
     */

    return 0;
}

int getCount( ) {

    int i,j;
    unsigned int byteOrder = 0;
    int	transaction = 0;



    for( i = 0; i < 8; i++ )
    {

	countSend.channel[i][0]	= (0x0000ff00 & node.cnt[i]) >> 8;
	countSend.channel[i][1]	= (0x000000ff & node.cnt[i]) >> 0;
	countSend.channel[i][2]	= (0xff000000 & node.cnt[i]) >> 24;
	countSend.channel[i][3]	= (0x00ff0000 & node.cnt[i]) >> 16;
    }


    tlvSendBuffer[0] = 0x01;
    tlvSendBuffer[1] = 0x02;
    tlvSendBuffer[2] = 0x00;
    tlvSendBuffer[3] = 0x00;
    tlvSendBuffer[4] = 0x00;
    tlvSendBuffer[5] = 0x23;    // t length
    tlvSendBuffer[6] = 0x01;
    tlvSendBuffer[7] = 0x04;
    tlvSendBuffer[8] = 0x20;

    memcpy(tlvSendBuffer+9, &countSend, sizeof( StatusSendData));

    int rtrn = write(client_sock, tlvSendBuffer, sizeof( StatusSendData)+9);
       for( i = 0; i < rtrn; i++ ) {
       printf("%-3.2x", tlvSendBuffer[i]);
       }
       printf("\n");

    memset( statusSend, 0, sizeof( StatusSendData)*8); 
    memset( tlvSendBuffer, 0, 2048 ); 


    return 0;
}


int getCommStatus( ) {

    int i,j;
    char *query;
    char *szErrMsg;
    int	rowSize = 8;
    int	rowCnt = 0;
    unsigned int byteOrder = 0;
    int	transaction = 0;

    SQLite3Data data;

    query = sqlite3_mprintf("select port1, port2, port3, port4, port5, port6, port7, port8 from tb_comm_status");	// edit 20140513


    data = _sqlite3_select( pSQLite3, query );


    if( data.size > 0 )
    {
	rowCnt = (data.size-8)/8;

	for( i = 0; i < (data.size-8)/8; i++ )
	{

	    for( j = 0; j < 8; j++ )
	    {
		byteOrder = atoi(data.data[rowSize++]);

		statusSend[i].channel[j][0]	= (0x0000ff00 & byteOrder) >> 8;
		statusSend[i].channel[j][1]	= (0x000000ff & byteOrder) >> 0;
		statusSend[i].channel[j][2]	= (0xff000000 & byteOrder) >> 24;
		statusSend[i].channel[j][3]	= (0x00ff0000 & byteOrder) >> 16;
	    }

	}

	tlvSendBuffer[0] = 0x01;
	tlvSendBuffer[1] = 0x02;
	tlvSendBuffer[2] = 0x00;
	tlvSendBuffer[3] = 0x00;
	tlvSendBuffer[4] = 0x00;
	tlvSendBuffer[5] = 0x23;    // t length
	tlvSendBuffer[6] = 0x01;
	tlvSendBuffer[7] = 0x04;
	tlvSendBuffer[8] = 0x20;

	memcpy(tlvSendBuffer+9, statusSend, sizeof( StatusSendData));

	int rtrn = write(client_sock, tlvSendBuffer, sizeof( StatusSendData)+9);
	/*
	   for( i = 0; i < rtrn; i++ ) {
	   printf("%-3.2x", tlvSendBuffer[i]);
	   }
	   printf("\n");
	 */

	memset( statusSend, 0, sizeof( StatusSendData)*8); 
	memset( tlvSendBuffer, 0, 2048 ); 

	sqlite3_free( query );
	SQLITE_SAFE_FREE( query )
	    sqlite3_free_table( data.data );
	SQLITE_SAFE_FREE( data.data )

    }

    return 0;
}

unsigned char _getCheckSum( int len )
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


void *thread_main(void *arg)
{

    writeLogV2(LOGPATH, "[thread_main]", "start Thread [SUBSCRIBE clear]\n");

    CONFIGINFO configInfo;

    configInfo = configInfoParser(CONFIGINFOPATH);


    debugPrintf(configInfo.debug, "start Thread [SUBSCRIBE clear]");
    struct event_base *base = event_base_new();
    redisAsyncContext *c = redisAsyncConnect("127.0.0.1", 6379);
    if (c->err) {
	// Let *c leak for now... 
	debugPrintf(configInfo.debug, "Error: %s\n", c->errstr);
	return;
    }
    redisLibeventAttach(c, base);
    redisAsyncCommand(c, onMessage, NULL, "SUBSCRIBE clear");
    event_base_dispatch(base);

    debugPrintf(configInfo.debug, "End Thread [SUBSCRIBE clear]");

    writeLogV2(LOGPATH, "[thread_main]", "End Thread [SUBSCRIBE clear]\n");
    pthread_exit((void *) 0);
}

int uartParsingReceiveValue(unsigned char* cvalue, int len, unsigned char* remainder, int remainSize )
{

    unsigned char setBuffer[BUFFER_SIZE];
    unsigned char sum = 0;
    int i;
    int offset;

    for( i = 0; i < len; i++ )
    {

	if( cvalue[i] == 0xFE )
	{

	    if( len >= i + (cvalue[i+1] + 3) && ( cvalue[i + (cvalue[i+1] + 2)] == 0xFF ) ) 
	    {
		// 데이터 수신정상
		memcpy( setBuffer, cvalue+i, (cvalue[i+1] + 3) );

		/*
		   for( offset = i; offset < i+(cvalue[i+1] + 3); offset++ )
		   printf("%02X ", cvalue[offset]);
		   printf("\n\n");
		 */


		i += (cvalue[i+1] + 2);

		remainSize = i+1;

		sum = _uartGetCheckSum( setBuffer, i+1 );

		if( sum != setBuffer[(i+1)-2] )
		{
		    //printf("================================\n");
		}
		else
		{

		    receiveEvent( setBuffer, i+1 );

		}

		sum = 0;

	    }

	}

    }

    remainSize = len - remainSize;
    //printf("reminSize(%d) = len(%d) - remainSize\n", remainSize, len);
    memcpy( remainder, cvalue+(len-(remainSize)), remainSize );


    return remainSize;
}


unsigned char _uartGetCheckSum( unsigned char* buffer, int len )
{
    int intSum = 0;
    unsigned char sum = 0;
    int i;

    //printf("start sum %d\n", len);
    for( i = 1; i< len-2; i++ )
    {
	sum += buffer[i];
	//intSum += buffer[i];
	//sleep(1);
	//printf("%d ", tlvSendBuffer[i]);
    }

    //sum = (unsigned char)intSum;
    //printf("sum %d\n", sum);
    return sum;
}

