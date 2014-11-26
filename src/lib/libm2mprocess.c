#include <stdio.h> 
#include <string.h>
#include <time.h>


#include "../include/M2MManager.h"
#include "../include/sqlite3.h"

#define SQLITE_SAFE_FREE(x)	if(x){ x = NULL; }

static int busy(void *handle, int nTry);

void writeTextLog( char* str )
{
	FILE    *fp_log;
	time_t 	curTime;
	struct tm *t;
	char     buff[1024]; // 파일 내요을 읽기/쓰기를 위한 버퍼

	//fp_log = fopen( "./rtd.log", "a");
	fp_log = fopen( "/work/m2m/m2m.log", "a");

	curTime = time(NULL);
	t = localtime(&curTime);

	sprintf( buff, "%d/%02d/%02d %02d:%02d:%02d - %s\n",
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



static int busy(void *handle, int nTry)
{
	//printf("%d th - busy handler is called\n", nTry);
	//usleep(10000);	// wait 10ms
	printf("M2M [%d] busy handler is called\n", nTry);
	//usleep(10000);	// wait 10ms

	//return 10-nTry;
	return 0;
}

int el1000_sqlite3_delete( sqlite3 *pSQLite3, char *query )
{
	int rst;
	char *szErrMsg;

	//printf("%s\n", query);
	rst = sqlite3_exec(
			pSQLite3,
			query,
			0,
			0,
			&szErrMsg);

	if( rst != SQLITE_OK )
	{
		printf("DELETE fail!! : %s\n", szErrMsg);
		sqlite3_free( szErrMsg );
		//writeTextLog( szErrMsg );
		SQLITE_SAFE_FREE( szErrMsg )
		return -1;
	}

	printf("delete rtrn = %d\n", rst);

	return 0;

}


//char** el1000_sqlite3_select( sqlite3 *pSQLite3, char *query )
Return_Selectdata el1000_sqlite3_select( sqlite3 *pSQLite3, char *query )
{
	int rst;
	int nrows 	= 0;
	int ncols	= 0;
	char *szErrMsg;
	char **past_result;

	Return_Selectdata data;

	//sqlite3_busy_handler( pSQLite3, busy, NULL);
	//sqlite3_busy_timeout( pSQLite3, 3000);

	//printf("Select !!\n");

	//query = sqlite3_mprintf("select groupid, nodeid, temperature, humidity, lux, bat from tb_th1000_data where nodeid = 1 order by id desc limit 1");

	//printf("%s\n", query);
	//rst = sqlite3_get_table(pSQLite3, query, &past_result, &nrows, &ncols, &szErrMsg);
	rst = sqlite3_get_table(pSQLite3, query, &data.past_result, &nrows, &ncols, &szErrMsg);

	if( rst != SQLITE_OK )
	{
		printf("%d\n", rst);
		printf("select fail!! : %s\n", szErrMsg);

		//writeTextLog( szErrMsg );
		sqlite3_free( szErrMsg );
		SQLITE_SAFE_FREE( szErrMsg )

		sqlite3_free_table( data.past_result );
		SQLITE_SAFE_FREE( data.past_result )

		// add 2013.10.01
		data.size = -1;
		return data;
	}

	data.size = (nrows * ncols + ncols);

	//sqlite3_free( query );
	//SQLITE_SAFE_FREE( query )


	//sqlite3_free_table( past_result );
	//SQLITE_SAFE_FREE( past_result )

	//return (nrows * ncols + ncols);
	return data;
}


Return_Selectdata getNodeInfo( sqlite3 *pSQLite3, char *query )
{
	int rst;
	int nrows 	= 0;
	int ncols	= 0;
	char *szErrMsg;

	Return_Selectdata data;


	printf("Select !!\n");

	rst = sqlite3_get_table(pSQLite3, query, &data.past_result, &nrows, &ncols, &szErrMsg);

	if( rst != SQLITE_OK )
	{
		printf("%d\n", rst);
		printf("select fail!! : %s\n", szErrMsg);
		sqlite3_free( szErrMsg );
		SQLITE_SAFE_FREE( szErrMsg )

		sqlite3_free_table( data.past_result );
		SQLITE_SAFE_FREE( data.past_result )

		// add 2013.10.01
		data.size = -1;
		return data;
	}

	data.size = (nrows * ncols + ncols);

	return data;
}
