#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <sys/types.h>
#include <sys/ipc.h>
#include <sys/msg.h>
#include <time.h>


#include "./include/iDCU.h"
#include "./include/sqlite3.h"
#include "./include/SQLite3Interface.h"
#include "./include/writeLog.h"

static int busy(void *handle, int nTry);

int main( void)
{
	int i;
	int rtrn;

	// sqlite
	int rc;
	sqlite3 *pSQLite3;
	char *query;
	char *delQuery;
	SQLite3Data sqlData;

	/************** DB connect.. ***********************/
	rc = IoT_sqlite3_open( "/work/db/comm", &pSQLite3 );
	printf("rc %d\n", rc );
	if( rc != 0 )
	{
	    writeLog( "/work/smart/log", "[DBSchedule] fail DB Opne" );
	    return -1;
	}
	else
	{
	    printf("DB OPEN!!\n");
	}


	/**********************************************/

	while( 1 )
	{

		query = sqlite3_mprintf("select strftime('%%Y-%%m-%%d %%H:%%M:%%f', 'now', '-7 days')"	);

		sqlite3_busy_handler( pSQLite3, busy, NULL);
		sqlData = IoT_sqlite3_select( pSQLite3, query );

		if( sqlData.size > 0 )
		{
			for( i = 1; i < sqlData.size; i++ )
			{
				printf("%s\n", sqlData.data[i] );

				delQuery = sqlite3_mprintf("delete from tb_comm_log where datetime < '%s'",
				sqlData.data[i]);
			}

			sqlite3_busy_handler( pSQLite3, busy, NULL);
			IoT_sqlite3_update( &pSQLite3, delQuery );   // delete 
		}

		sleep(60);
	}
}

static int busy(void *handle, int nTry)
{
    if( nTry > 19 )
    {
	printf("%d th - busy handler is called\n", nTry);
    }
    usleep(10000);	// wait 10ms

    return 20-nTry;
}

