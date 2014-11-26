#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <sys/types.h>
#include <sys/ipc.h>
#include <sys/msg.h>
#include <time.h>
#include <sys/reboot.h>


#include "./include/sqlite3.h"
#include "./include/RealTimeDataManager.h"

extern int el1000_sqlite3_open( char *path, sqlite3 **pSQLite3 );
extern int el1000_sqlite3_customize( sqlite3 **pSQLite3 );
extern int el1000_sqlite3_close( sqlite3 **pSQLite3 );
extern int el1000_sqlite3_transaction( sqlite3 **pSQLite3, char status );
extern Return_Selectdata el1000_sqlite3_select( sqlite3 *pSQLite3, char *query );
extern int InsertAustemNodeData( sqlite3 **pSQLite3, t_getNode	node );
extern int InsertAustemNodeStatusData( sqlite3 **pSQLite3, t_getNode	node );
extern int UpdateAustemNodeData( sqlite3 **pSQLite3, t_getNode	node );
static int busy(void *handle, int nTry);
int commandDeleteNode( sqlite3 *pSQLite3, unsigned char nodeid );
int commandUpdateNode( sqlite3 *pSQLite3, unsigned char nodeid, unsigned char changenodeid, unsigned char groupid );

/*
   void writeLog( char* str )
   {
   FILE    *fp_log;
   time_t 	curTime;
   struct tm *t;
   char     buff[1024]; // 파일 내요을 읽기/쓰기를 위한 버퍼

   fp_log = fopen( "/work/rtd/rtd.log", "a");

   curTime = time(NULL);
   t = localtime(&curTime);

   sprintf( buff, "%d/%d/%d %d:%d:%d - %s\n",
   t->tm_year + 1900,
   t->tm_mon + 1,
   t->tm_mday,
   t->tm_hour,
   t->tm_min,
   t->tm_sec,
   str);
   fwrite( buff, 1, strlen(buff), fp_log );

   fclose( fp_log );
   }
 */

void writeLog( char* str )
{
    FILE    *fp_log;
    struct timeval val;
    struct tm *t;
    char     buff[1024]; // 파일 내요을 읽기/쓰기를 위한 버퍼

    fp_log = fopen( "/work/rtd/rtd.log", "a");


    gettimeofday(&val, NULL);
    t = localtime(&val.tv_sec);

    sprintf( buff, "%04d/%02d/%02d %02d:%02d:%02d.%03d - %s\n",
	    t->tm_year + 1900,
	    t->tm_mon + 1,
	    t->tm_mday,
	    t->tm_hour,
	    t->tm_min,
	    t->tm_sec,
	    val.tv_usec/1000,
	    str);

    fwrite( buff, 1, strlen(buff), fp_log );

    fclose( fp_log );
}

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
    t_node	value[256];
    t_getNode	node;
    t_getNode	lossNode;
    int loss[DI_PORT];
    int lossOffset = 0;
    char lossCheckStart[256];

    // sqlite
    int rc;
    sqlite3 *pSQLite3;
    int transaction = 0;
    //NODELIST nodelist;
    unsigned char nodelist[256];
    char *query;
    Return_Selectdata selectLastData;
    int	getPortOffset = 0;
    //int	dataChanged = 0;
    int	packetErr = 0;
    unsigned char checkSumCompare = 0;

    //clock_t start,end;
    //double msec;

    memset( &data, 0, sizeof(t_data) );
    memset( &eventData, 0, sizeof(t_data) );
    memset( value, 0, (sizeof( t_node ) * 256) ); 
    memset( nodelist, 0, 256 ); 
    memset( lossCheckStart, 0, 256 ); 
    memset( loss, 0, (sizeof(int) * DI_PORT) ); 

    //sleep(4);

    if ( -1 == ( msqid = msgget( (key_t)1111, IPC_CREAT | 0666)))
    {
	writeLog( "error msgget() msqid" );
	//perror( "msgget() 실패");
	exit( 1);
    }
    if( -1 == ( eventid = msgget( (key_t)3333, IPC_CREAT | 0666)))
    {
	writeLog( "error msgget() eventid" );
	//perror( "msgget() 실패");
	return -1;
    }


    /************** DB connect.. ***********************/
    // DB Open
    rc = el1000_sqlite3_open( DBPATH, &pSQLite3 );
    //rc = el1000_sqlite3_open( argv[1], &pSQLite3 );
    if( rc != 0 )
    {
	writeLog( "error DB Open" );
	return -1;
    }
    else
	printf("%s OPEN!!\n", DBPATH);
    //printf("%s OPEN!!\n", argv[1]);

    // DB Customize
    rc = el1000_sqlite3_customize( &pSQLite3 );
    if( rc != 0 )
    {
	writeLog( "error DB Customize" );
	return -1;
    }

    //sqlite3_busy_handler( pSQLite3, busy, NULL);
    //sqlite3_busy_timeout( pSQLite3, 3000);
    //sqlite3_busy_timeout( pSQLite3, 1000);


    /**********************************************/

    //start = clock();
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
	    cntOffset = 0;
	    for( i = 0; i < DI_PORT; i++ )
	    {
		node.cnt[i] = 0;

		node.cnt[i] = data.data_buff[cntOffset++] << 24;
		node.cnt[i] |= data.data_buff[cntOffset++] << 16;
		node.cnt[i] |= data.data_buff[cntOffset++] << 8;
		node.cnt[i] |= data.data_buff[cntOffset++] << 0;

	    }

	    printf("%d %d %d %d %d %d %d %d\n",
		    node.cnt[0],
		    node.cnt[1],
		    node.cnt[2],
		    node.cnt[3],
		    node.cnt[4],
		    node.cnt[5],
		    node.cnt[6],
		    node.cnt[7] );

	    do 
	    {
		transaction = el1000_sqlite3_transaction( &pSQLite3, INSERT_BEGIN );

	    }while( transaction );

	    UpdateAustemNodeData( &pSQLite3, node );

	    do
	    {
		transaction = el1000_sqlite3_transaction( &pSQLite3, INSERT_END ) ;

	    } while( transaction );

	}


	memset( &eventData, 0, sizeof(t_data) );
	// event receive ( delete node, command reboot )
	if( -1 == msgrcv( eventid, &eventData, sizeof( t_data) - sizeof( long), 0, IPC_NOWAIT) )
	{
	    // error
	}
	else
	{

	    if( eventData.data_num != eventData.data_buff[1] + 3 )
		printf("packet error ( size %d == %d )\n", eventData.data_num, eventData.data_buff[1]);
	    else
	    {

		for( i = 0; i < eventData.data_num; i++ )
		    printf("%02X ", eventData.data_buff[i]);
		printf("\n");
		switch(  eventData.data_buff[2] )
		{
		    //reboot
		    case 1:
			//reboot(RB_AUTOBOOT);
			execl("/sbin/reboot", "/sbin/reboot", NULL);
			break;
			//CHANGENODEINFO
		    case 18:
			/*
			   printf("exist Clear : %d\n", eventData.data_buff[4]);
			   value[eventData.data_buff[4]].exist = 0;
			   lossCheckStart[eventData.data_buff[4]] = 0;
			   value[eventData.data_buff[5]].exist = 1;
			   do 
			   {
			   transaction = el1000_sqlite3_transaction( &pSQLite3, INSERT_BEGIN );
			   }while( transaction );
			   commandUpdateNode( pSQLite3, eventData.data_buff[4], eventData.data_buff[5], eventData.data_buff[6] );
			   do
			   {
			   transaction = el1000_sqlite3_transaction( &pSQLite3, INSERT_END ) ;
			   } while( transaction );
			   break;
			 */
			//DELETENODE
		    case 4:
			printf("exist Clear : %d\n", eventData.data_buff[4]);
			value[eventData.data_buff[4]].exist = 0;
			lossCheckStart[eventData.data_buff[4]] = 0;
			do 
			{
			    transaction = el1000_sqlite3_transaction( &pSQLite3, INSERT_BEGIN );
			}while( transaction );
			commandDeleteNode( pSQLite3, eventData.data_buff[4] );
			do
			{
			    transaction = el1000_sqlite3_transaction( &pSQLite3, INSERT_END ) ;
			} while( transaction );

			break;
		}
	    }
	}


    }
}

int commandUpdateNode( sqlite3 *pSQLite3, unsigned char nodeid, unsigned char changenodeid, unsigned char groupid )
{
    char* updateQuery;

    updateQuery = sqlite3_mprintf("update tb_comm_status set nodeid = '%d', groupid = '%d' where nodeid = '%d'",
	    changenodeid,
	    groupid,
	    nodeid
	    );

    el1000_sqlite3_delete( pSQLite3, updateQuery );

    sqlite3_free( updateQuery );
    SQLITE_SAFE_FREE( updateQuery )

	return 0;
}

int commandDeleteNode( sqlite3 *pSQLite3, unsigned char nodeid )
{
    char* delQuery;

    delQuery = sqlite3_mprintf("delete from tb_comm_status where nodeid = '%d'",
	    nodeid);

    el1000_sqlite3_delete( pSQLite3, delQuery );

    sqlite3_free( delQuery );
    SQLITE_SAFE_FREE( delQuery )

	return 0;
}

static int busy(void *handle, int nTry)
{
    printf("[%d] busy handler is called\n", nTry);

    //writeLog( "DB busy : call sqlite3_busy_handler" );
    return 0;

}
