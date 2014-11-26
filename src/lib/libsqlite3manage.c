#include <stdio.h> 
#include <stdlib.h> 
#include <string.h>
#include <time.h>


//#include "../include/RealTimeDataManager.h"
#include "../include/iDCU.h"
#include "../include/sqlite3.h"

#define SQLITE_SAFE_FREE(x)	if(x){ x = NULL; }

static int busy(void *handle, int nTry);


static int busy(void *handle, int nTry)
{
	printf("%d th - busy handler is called\n", nTry);
	usleep(10000);	// wait 10ms

	return 10-nTry;
}

/*
void writeSqliteLog( char* str )
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
*/

int el1000_sqlite3_open( char *path, sqlite3 **pSQLite3 )
{
	int rc;

	rc = sqlite3_open( path, &*pSQLite3);
	if ( rc != SQLITE_OK )
	{
		fprintf(stderr, "Error code is %d\n", rc);
		printf( "Can't open database: %s\n", sqlite3_errmsg( *pSQLite3 ));
		return -1;
	}
	return 0;

}

int el1000_sqlite3_customize( sqlite3 **pSQLite3 )
{
	int rc;
	char *szErrMsg;

	//sqlite3_busy_handler( *pSQLite3, busy, NULL);

	rc = sqlite3_exec(
			*pSQLite3, 
			"PRAGMA auto_vacuum = 1;",
			NULL, 
			NULL, 
			&szErrMsg);

	if( rc != SQLITE_OK )
	{
		printf("Custumize Error : %s\n", szErrMsg);
		sqlite3_free( szErrMsg );
		if( szErrMsg != NULL )
			szErrMsg = NULL;

		return -1;
	}
	return 0;
}	


int el1000_sqlite3_close( sqlite3 **pSQLite3 )
{

	int rc;


	rc = sqlite3_close( *pSQLite3 );
	if ( rc != SQLITE_OK )
	{
		return rc;
	}
	printf("SUCCESS DB handle close!!\n");

	return rc;
}

int el1000_sqlite3_transaction( sqlite3 **pSQLite3, char status )
{

	int rc;
	char *szErrMsg;

	//sqlite3_busy_handler( *pSQLite3, busy, NULL);

	switch( status )
	{
		case INSERT_BEGIN:
			rc = sqlite3_exec( *pSQLite3, "BEGIN EXCLUSIVE TRANSACTION;", NULL, NULL, &szErrMsg );
			if( rc != SQLITE_OK )
			{
				printf("[%d] BEGIN TRANSACTION : %s\n", rc, szErrMsg);
				sqlite3_free( szErrMsg );
				SQLITE_SAFE_FREE( szErrMsg )
				return -1;
			}
			break;

		case INSERT_END:
			rc = sqlite3_exec( *pSQLite3, "END TRANSACTION;", NULL, NULL, &szErrMsg );
			if( rc == SQLITE_ERROR )
			{
				//writeSqliteLog( szErrMsg );
				sqlite3_free( szErrMsg );
				SQLITE_SAFE_FREE( szErrMsg )
				return 0;
			}
			if( rc != SQLITE_OK )
			{
				printf("[%d] END TRANSACTION : %s\n", rc, szErrMsg);
				sqlite3_free( szErrMsg );
				SQLITE_SAFE_FREE( szErrMsg )
				return -1;
			}
			break;
	}

	return 0;
}

int el1000_sqlite3_update( sqlite3 **pSQLite3, char* query )
{
	char	*szErrMsg;
	int rst;

	rst = sqlite3_exec(
			*pSQLite3,
			query,
			0,
			0,
			&szErrMsg);

	if( rst != SQLITE_OK )
	{
		printf("update fail!! : %s\n", szErrMsg);
		//writeSqliteLog( szErrMsg );
		sqlite3_free( szErrMsg );
		if( szErrMsg != NULL )
			szErrMsg = NULL;
	}

	sqlite3_free( query );		// mem free
	if( query != NULL )
		query = NULL;

	return 0;
}

