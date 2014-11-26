#include <stdio.h> 
#include <stdlib.h> 
#include <string.h>
#include <time.h>

#include "../include/iDCU.h"
#include "../include/sqlite3.h"

#define SQLITE_SAFE_FREE(x)	if(x){ x = NULL; }

int _sqlite3_update( sqlite3 **pSQLite3, MGData node )
{
    char *log_buf;
    char	*szErrMsg;
    int rst;

    log_buf = sqlite3_mprintf("UPDATE TB_COMM_STATUS SET datetime = strftime('%%Y-%%m-%%d %%H:%%M:%%f', 'now','localtime'), port1 = %d, port2 = %d, port3 = %d, port4 = %d, port5 = %d, port6 = %d, port7 = %d, port8 = %d",
	    node.cnt[0],
	    node.cnt[1],
	    node.cnt[2],
	    node.cnt[3],
	    node.cnt[4],
	    node.cnt[5],
	    node.cnt[6],
	    node.cnt[7]
	    );
    rst = sqlite3_exec(
	    *pSQLite3,
	    log_buf,
	    0,
	    0,
	    &szErrMsg);

    if( rst != SQLITE_OK )
    {
	printf("update fail!! : %s\n", szErrMsg);
	sqlite3_free( szErrMsg );
	if( szErrMsg != NULL )
	    szErrMsg = NULL;
    }

    sqlite3_free( log_buf );		// mem free
    if( log_buf != NULL )
	log_buf = NULL;


    return 0;
}

SQLite3Data _sqlite3_select( sqlite3 *pSQLite3, char *query )
{
    int rst;
    int nrows 	= 0;
    int ncols	= 0;
    char *szErrMsg;

    SQLite3Data data;

    rst = sqlite3_get_table(pSQLite3, query, &data.data, &nrows, &ncols, &szErrMsg);

    if( rst != SQLITE_OK )
    {
	printf("%d\n", rst);
	printf("select fail!! : %s\n", szErrMsg);

	sqlite3_free( szErrMsg );
	SQLITE_SAFE_FREE( szErrMsg )

	    sqlite3_free_table( data.data);
	SQLITE_SAFE_FREE( data.data)

	    // add 2013.10.01
	    data.size = -1;
	return data;
    }

    data.size = (nrows * ncols + ncols);

    return data;
}



int _sqlite3_open( char *path, sqlite3 **pSQLite3 )
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

int _sqlite3_nolock( sqlite3 **pSQLite3 )
{
    int rc;
    char *szErrMsg;


    rc = sqlite3_exec(
	    *pSQLite3, 
	    "PRAGMA read_uncommitted = 1;",
	    NULL, 
	    NULL, 
	    &szErrMsg);

    if( rc != SQLITE_OK )
    {
	printf("read_uncommitted Error : %s\n", szErrMsg);
	sqlite3_free( szErrMsg );
	if( szErrMsg != NULL )
	    szErrMsg = NULL;

	return -1;
    }
    return 0;
}	


int _sqlite3_customize( sqlite3 **pSQLite3 )
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


int _sqlite3_close( sqlite3 **pSQLite3 )
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

int _sqlite3_transaction( sqlite3 **pSQLite3, char status )
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
