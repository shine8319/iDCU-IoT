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
#include <sched.h>

#include "./include/iDCU.h"
#include "./include/sqlite3.h"
#include "./include/SQLite3Interface.h"
#include "./include/writeLog.h"
#include "./include/TCPSocket.h"
#include "./include/configRW.h"
#include "./include/ETRI.h"
//#include "./include/ETRI_Registration.h"
//#include "./include/ETRI_Deregistration.h"

#define BUFFER_SIZE 1024
#define TABLE_PATH  "TB_COMM_LOG"
#define EVENT_Q_SIZE 64 

static int busy(void *handle, int nTry);
static void *thread_insert(void *arg);

static sqlite3 *pSQLite3;

static pthread_t threads;

static unsigned char strPac[EVENT_Q_SIZE][BUFFER_SIZE*2];
static UINT8 eventCount;
static UINT8 eventIn;
static UINT8 eventOut;

int main( void)
{
    Struct_SensingValueReport pac;
    READENV env;
    int      msqid;
    int      eventid;
    int 		i,j;
    int 		id = 0;
    int			rtrn;
    int			reTry = 3;
    char		cntOffset = 9;
    t_data   data;
    t_data   eventData;
    time_t  transferTime, sensingTime;

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



    if ( -1 == ( msqid = msgget( (key_t)1, IPC_CREAT | 0666)))
    {
	writeLog( "/work/smart/log", "[tagServer] error msgget() msqid" );
	return -1;
    }

    //memcpy( th_data, (void *)pSQLite3, sizeof(pSQLite3) );
    if( pthread_create(&threads, NULL, &thread_insert, (void *)th_data) == -1 )
    {
	    writeLog("/work/smart/log", "[tagServer] error pthread_create method" );
    }

    ReadEnvConfig("/work/smart/reg/env.reg", &env );

    unsigned char savePac[1024];

    writeLog( "/work/smart/log", "[tagServer] Start tagServer.o" );

    eventCount = 0;
    memset( strPac, 0, sizeof( strPac ) );

    while( 1 )
    {

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



	    int bufferOffset = 0;

	    if( data.data_type == 1 )
	    {

		if( eventCount < EVENT_Q_SIZE )
		{


		    pac.Message_Id		= 0x09;
		    pac.Length		= (SENSING_VALUE_REPORT_HEAD_SIZE-4) + data.data_num;
		    pac.Command_Id		= 0xFFFFFFFF;
		    pac.GateNode_Id		= env.gatenode;
		    pac.PAN_Id			= env.pan;
		    pac.SensorNode_Id		= env.sensornode; 
		    /*
		    printf(" gata %d, pan %d, node %d\n", pac.GateNode_Id
			    ,pac.PAN_Id
			    ,pac.SensorNode_Id );
			    */

		    time(&transferTime);
		    pac.Transfer_Time		= transferTime;

		    memcpy( pac.Transducer_Info, data.data_buff, data.data_num );

		    memset( savePac, 0, sizeof( savePac ) );

		    memcpy( savePac, &pac, SENSING_VALUE_REPORT_HEAD_SIZE + data.data_num );
		    for( i = 0; i < SENSING_VALUE_REPORT_HEAD_SIZE + data.data_num; i++ )
		    {
			//printf("%02X ", savePac[i]);
			sprintf( strPac[eventIn]+(i*2), "%02X ", savePac[i]); 
		    }
		    //printf("\n");

		    eventCount++;
		    if( ++eventIn >= EVENT_Q_SIZE ) eventIn = 0;

		}
		else
		{
		    // var init
		    eventCount = 0;
		    eventIn = 0;
		    eventOut = 0;
		    memset( strPac, 0, sizeof( strPac ) );

		    writeLog( "/work/smart/log", "[tagServer] full Q" );
		}

	    }

	}

    }

}

static void *thread_insert(void *arg)
{

    int rtrn;
    //sqlite3 *Fd;
    char *query;


    while(1)
    {

	if( eventCount != 0)
	{
	    //printf("eventCount %d\n", eventCount);
	    query = sqlite3_mprintf("insert into '%s' ( datetime, value, tag ) \
		    values ( datetime('now','localtime'), '%s', '0' )",
		    TABLE_PATH,
		    strPac[eventOut]
		    );
	    printf("%s\n", query );

	    sqlite3_busy_handler( pSQLite3, busy, NULL);
	    rtrn = IoT_sqlite3_insert( &pSQLite3, query );
	    if( rtrn == 0 )
	    {
		// success
		eventCount--;
		if( ++eventOut >= EVENT_Q_SIZE ) eventOut = 0;
	    }
	    else
		writeLog( "/work/smart/log", "[tagServer] fail insert" );


	}
	else
	{
	    //printf("no Data (eventCount %d)\n", eventCount );
	    usleep(500000);	// 500ms
	}

    }

}

static int busy(void *handle, int nTry)
{
    if( nTry > 9 )
    {
	//printf("%d th - busy handler is called\n", nTry);
    }
    usleep(10000);	// wait 10ms

    return 10-nTry;
}



int set_highest_priority (pid_t pid)
{
    struct sched_param sp;
    int policy, max, ret;
    policy = sched_getscheduler (pid); // pid의 스케줄링 정책을 얻는다.
    if (policy == -1)
	return -1;

    max = sched_get_priority_max (policy); // 최대 우선순위를 구한다.
    if (max == -1)
	return -1;

    memset (&sp, 0, sizeof (struct sched_param));
    sp.sched_priority = max;
    ret = sched_setparam (pid, &sp); // 우선순위를 갱신한다.

    return ret;
}
/*
    //set_highest_priority(0);
    struct sched_param sp = { .sched_priority = 1 };
    int ret;
    ret = sched_setscheduler (0, SCHED_RR, &sp);
    if (ret == -1) {
	perror ("sched_setscheduler");
	//return 1;
    }
 
    int policy;
    // 현재 프로세스의 스케줄링 정책 얻기
    policy = sched_getscheduler (0);
    switch (policy) {
	case SCHED_OTHER:
	    printf ("main Policy is normal\n");
	    break;
	case SCHED_RR:
	    printf ("main Policy is round-robin\n");
	    break;
	case SCHED_FIFO:
	    printf ("main Policy is first-in, first-out\n");
	    break;
	case -1:
	    perror ("main sched_getscheduler");
	    break;
	default:
	    fprintf (stderr, "main Unknown policy!\n");
    }

    */

