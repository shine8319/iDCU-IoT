#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <sys/types.h>
#include <sys/ipc.h>
#include <sys/msg.h>
#include <time.h>


#define LOGPATH "/work/log/DBSchedule"
#include "./include/sqlite3.h"
#include "./include/RealTimeDataManager.h"

extern int el1000_sqlite3_open( char *path, sqlite3 **pSQLite3 );
extern int el1000_sqlite3_customize( sqlite3 **pSQLite3 );
extern int el1000_sqlite3_close( sqlite3 **pSQLite3 );
extern int el1000_sqlite3_transaction( sqlite3 **pSQLite3, char status );
extern Return_Selectdata el1000_sqlite3_select( sqlite3 *pSQLite3, char *query );
//extern int InsertAustemNodeData( sqlite3 **pSQLite3, t_getNode	node );
//extern int UpdateAustemNodeData( sqlite3 **pSQLite3, t_getNode	node );
static int busy(void *handle, int nTry);


int main( void)
{
	int      msqid;
	int 		i,j;
	int 		id = 0;
	int			rtrn;
	char		cntOffset = 9;

	// sqlite
	int rc;
	sqlite3 *pSQLite3;
	//NODELIST nodelist;
	unsigned char nodelist[256];
	char *query;
	char *delQuery;
	Return_Selectdata selectLastData;
	int	getPortOffset = 0;
	int	dataChanged = 0;

	clock_t start,end;
	double msec;

	memset( nodelist, 0, 256 ); 

	if ( -1 == ( msqid = msgget( (key_t)1111, IPC_CREAT | 0666)))
	{
		//writeLog( "error msgget() msqid" );
		writeLogV2(LOGPATH, "[DBSchedule]", "error msgget() msqid\n");
		//perror( "msgget() ����");
		exit( 1);
	}

	/************** DB connect.. ***********************/
	// DB Open
	rc = el1000_sqlite3_open( DBPATH, &pSQLite3 );
	//rc = el1000_sqlite3_open( argv[1], &pSQLite3 );
	if( rc != 0 )
	{
		//writeLog( "error DB Open" );
		writeLogV2(LOGPATH, "[DBSchedule]", "error DB Open\n");
		return -1;
	}
	else
	{
		printf("%s OPEN!!\n", DBPATH);
		writeLogV2(LOGPATH, "[DBSchedule]", "%s OPEN!!\n", DBPATH);
		//printf("%s OPEN!!\n", argv[1]);
	}

	// DB Customize
	rc = el1000_sqlite3_customize( &pSQLite3 );
	if( rc != 0 )
	{
		//writeLog( "error DB Customize" );
		writeLogV2(LOGPATH, "[DBSchedule]", "error DB Customize\n");
		return -1;
	}

	sqlite3_busy_handler( pSQLite3, busy, NULL);
	sqlite3_busy_timeout( pSQLite3, 3000);


	/**********************************************/

	while( 1 )
	{

		query = sqlite3_mprintf("select strftime('%%Y-%%m-%%d %%H:%%M:%%f', 'now', '-7 days')"	);
		selectLastData = el1000_sqlite3_select( pSQLite3, query );
		//printf("select size = %d\n", selectLastData.size );

		if( selectLastData.size > 0 )
		{
			for( i = 1; i < selectLastData.size; i++ )
			{
				//printf("%s\n", selectLastData.past_result[i] );

				delQuery = sqlite3_mprintf("delete from tb_comm_log where datetime < '%s'",
					selectLastData.past_result[i]);
			}
			el1000_sqlite3_delete( pSQLite3, delQuery );
		}

		sqlite3_free( query );
		sqlite3_free( delQuery );
		SQLITE_SAFE_FREE( query )
		SQLITE_SAFE_FREE( delQuery )
		sqlite3_free_table( selectLastData.past_result );
		SQLITE_SAFE_FREE( selectLastData.past_result )

		//sleep(60);
		sleep(180);
	}
}

static int busy(void *handle, int nTry)
{
	printf("[%d] busy handler is called\n", nTry);
	return 0;

}
