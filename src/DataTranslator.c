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
#include "./include/SQLite3Interface.h"
#include "./include/writeLog.h"
#include "./include/TCPSocket.h"
#include "./include/configRW.h"
#include "./include/ETRI.h"
#include "./include/ETRI_Registration.h"
#include "./include/ETRI_Deregistration.h"
#include "./include/hexString.h"

#define SQLITE_SAFE_FREE(x)     if(x){ x = NULL; }

#define BUFFER_SIZE 1024
#define TABLE_PATH  "TB_COMM_LOG"
int tcp;
pthread_t threads[2];
int pushStart;
READENV env;

static int init(void);
void *thread_sender(void *arg);
void *thread_receive(void *arg);
Struct_SensingValueReport pac;
static int busy(void *handle, int nTry);

int main( void)
{
    //int      msqid;
    int      eventid;
    int 		i,j;
    int 		id = 0;
    int			rtrn;
    int			reTry = 3;
    char		cntOffset = 9;
    t_data   data;
    t_data   eventData;
    time_t  transferTime, sensingTime;

    char *query;
    char *updateQuery;
    int	getPortOffset = 0;
    //int	dataChanged = 0;
    int	packetErr = 0;
    unsigned char checkSumCompare = 0;
    UINT8 sendBuffer[1024];
    //clock_t start,end;
    //double msec;

    //thread
    char th2_data[256];

    sqlite3 *pSQLite3;
    UINT32 rc;

    memset( &env, 0, sizeof( READENV ) );
    memset( &data, 0, sizeof(t_data) );
    memset( &eventData, 0, sizeof(t_data) );
    memset( sendBuffer, 0, 1024 );

    rc = IoT_sqlite3_open( "/work/db/comm", &pSQLite3 );
    printf("rc %ld\n", rc );
    if( rc != 0 )
    {
	writeLog( "/work/smart/log", "[tagServer] fail DB Opne" );
	return -1;
    }
    else
    {
	printf("DB OPEN!!\n");
    }



    while( reTry )
    {
	rtrn = init();
	if( rtrn == -1 )
	{
	    printf("reTry %d\n", reTry);
	    reTry--;
	    sleep(10);
	}
	else
	    break;

	if( reTry == 0 )
	    return -1;
    }

    int ReadMsgSize;
    unsigned char DataBuf[1024];
    unsigned char sendBuf[1024];

    unsigned char savePac[1024];
    unsigned int strPac[2048];

    int selectOffset; 
    SQLite3Data sqlData;
    //uint8_t str_len;
    int str_len;
    //const char *pos;
    char *pos;
    int	idIndex;
    //unsigned char val[10240];
    unsigned char *val;
    size_t count = 0;
    int sqlRtrn;

    writeLog( "/work/smart/log", "[DataTranslator] Start DataTranslator.o" );
    //query = sqlite3_mprintf("select id, value from tb_comm_log where tag != 1 order by id asc limit 1");
    query = sqlite3_mprintf("select id, value from tb_comm_log order by datetime asc limit 1");


    while( 1 )
    {

	// disconnect check
	if( tcp == -1 )
	{
	    rtrn = init();
	    printf("disconnect => %d = init();\n",rtrn);
	    if( rtrn == -1 )
	    {

		printf("exit~~~~~~~~~~~~~\n");
		return -1;
	    }
	}

	sqlite3_busy_handler( pSQLite3, busy, NULL);
	sqlData = IoT_sqlite3_select( pSQLite3, query );

	selectOffset = 2;


	if( sqlData.size > 0 )
	{

	    printf(" select size = %ld\n", sqlData.size );
	    idIndex = atoi(sqlData.data[selectOffset++]);	// just one incresment

	    //pos = sqlData.data[selectOffset];
	    str_len = strlen(sqlData.data[selectOffset]);
	    printf(" str_len %d\n", str_len);
	    val = hexStringToBytes(sqlData.data[selectOffset]);
	    /* WARNING: no sanitization or error-checking whatsoever */
	    /*
	    for(count = 0; count < str_len; count++) {
		sscanf(pos, "%2hhx", &val[count]);
		pos += 2 * sizeof(char);
	    }
	    printf("0x");

	    */
	    for(count = 0; count < str_len/2; count++)
		printf("%02X", val[count]);
	    printf("\n");

	    
	    time(&transferTime);
	    memcpy( val+14, &transferTime, 4 );
	    rtrn = send(tcp, val, count, MSG_NOSIGNAL);

	    if( rtrn == -1 )
	    {

    
		close(tcp);
		tcp = -1;
		printf("close(tcp) %d\n", tcp);
		sleep(10);

		rtrn = init();
		if( rtrn == -1 )
		{

		    printf("exit~~~~~~~~~~~~~\n");
		    return -1;
		}
	    }
	    else
	    {
		printf("send length = %d\n", rtrn);
		memset( sendBuf, 0, 1024);
		memcpy( sendBuf, &pac, rtrn );
		for( i = 0; i < rtrn; i++ )
		    printf("%02X ", val[i]);
		printf("\n");

		/*
		updateQuery = sqlite3_mprintf("update '%s' set tag = 1 \
						where id = '%d'",
				TABLE_PATH,
				idIndex	
				);
				*/
		updateQuery = sqlite3_mprintf("delete from '%s'	where id = '%d'",
				TABLE_PATH,
				idIndex	
				);

		printf("%s\n", updateQuery );

		sqlite3_busy_handler( pSQLite3, busy, NULL);
		sqlRtrn = IoT_sqlite3_update( &pSQLite3, updateQuery );
		printf("delete sqlRtrn = %d\n", sqlRtrn);


	    }
	    free(val);	// malloc in hexStringToBytes();
	    sqlite3_free_table( sqlData.data );
	    SQLITE_SAFE_FREE( sqlData.data )

	    usleep(100000);	// 100ms


	}
	else
	{
	    usleep(500000);	// 500ms
	}


    }

}

static int busy(void *handle, int nTry)
{
    if( nTry > 9 )
    {
	printf("%d th - busy handler is called\n", nTry);
    }
	usleep(10000);	// wait 10ms

	return 10-nTry;
}

static int init(void)
{
    char th_data[256];
    struct sockaddr_in ip; 
    int reTry = 3;

    ReadEnvConfig("/work/smart/reg/env.reg", &env );

    ip.sin_addr.s_addr = (env.middleware_ip);
    printf(" bind IP %s, port %d\n", inet_ntoa(ip.sin_addr), env.middleware_port);


    do {

	if( !(tcp > 0) )
	{
	    tcp = TCPClient( inet_ntoa(ip.sin_addr), env.middleware_port);
	    printf(" tcp status %d\n", tcp );
	    if( tcp < 0 )
	    {
		writeLog("/work/smart/log", "[DataTranslator] fail TCPClient method " );
		printf("connect error\n");
		return -1;
	    }
	    else
	    {
		printf("connect success\n");
	    }

	}


	if( -1 == ETRI_Registration( &tcp, &env ) )
	{
	    //printf("fail registration\n");
	    writeLog( "/work/smart/log", "[tagServer] fail registration" );
	    //return -1;
	    sleep(1);
	    reTry--;
	    if( reTry == 0 )
	    {
		close(tcp);
		tcp = -1;
		return -1;
	    }
	}
	else
	{
	    reTry = 0;
	    memcpy( th_data, (void *)&tcp, sizeof(tcp) );
	    if( pthread_create(&threads[0], NULL, &thread_receive, (void *)th_data) == -1 )
	    {
		writeLog("/work/smart/log", "[DataTranslator] error pthread_create method" );
	    }

	}

    } while( reTry );

    return 0;
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

    //memcpy( (void *)&Fd, (char *)arg, sizeof(int));

    writeLog("/work/smart/log", "[DataTranslator] Start thread_receive" );
    printf("thread start polling time %ldsec\n", env.timeout);

    FD_ZERO(&control_msg_readset);
    //printf("Receive Ready!!!\n");

    while( 1 ) 
    {

	FD_SET(tcp, &control_msg_readset);
	control_msg_tv.tv_sec = env.timeout;
	//control_msg_tv.tv_usec = 10000;
	control_msg_tv.tv_usec = 0;	// timeout check 

	nd = select( tcp+1, &control_msg_readset, NULL, NULL, &control_msg_tv );		
	if( nd > 0 ) 
	{

	    memset( DataBuf, 0, sizeof(DataBuf) );
	    ReadMsgSize = recv( tcp, &DataBuf, BUFFER_SIZE, MSG_DONTWAIT);
	    if( ReadMsgSize > 0 ) 
	    {
		for( i = 0; i < ReadMsgSize; i++ ) {
		    printf("%-3.2x", DataBuf[i]);
		}
		printf("\n");
		printf("recv data size : %d\n", ReadMsgSize);

		if( ReadMsgSize >= BUFFER_SIZE )
		    continue;

	    } 
	    else {
		sleep(1);
		printf("[DataTranslator] Server Disconnect %d\n", ReadMsgSize);
		writeLog( "/work/smart/log", "[DataTranslator] Server Disconnect" );
		close(tcp);
		tcp = -1;
		break;
	    }
	    ReadMsgSize = 0;

	} 
	/*
	else if( nd == 0 ) 
	{
	    printf("[DataTranslator] timeout %ldsec\n", env.timeout );
	    //shutdown( *tcp, SHUT_WR );
	    pushStart = 1;
	    close(Fd);
	    break;
	}
	*/
	else if( nd == -1 ) 
	{
	    printf("[DataTranslator] error...................\n");
	    writeLog( "/work/smart/log", "[DataTranslator] connect error............" );
	    close(tcp);
	    tcp = -1;
	    break;
	}
	nd = 0;

    }	// end of while

    writeLog("/work/smart/log", "[DataTranslator] Exit thread_receive" );
    printf("thread Exit\n");

}
