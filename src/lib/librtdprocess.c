#include <stdio.h> 
#include <stdlib.h> 
#include <string.h>
#include <time.h>


#include "../include/RealTimeDataManager.h"
#include "../include/sqlite3.h"

#define SQLITE_SAFE_FREE(x)	if(x){ x = NULL; }
#define TABLE_PATH "tb_comm_log"

static int busy(void *handle, int nTry);
void writeTextLog( char* str )
{
	FILE    *fp_log;
	time_t 	curTime;
	struct tm *t;
	char     buff[1024]; // 파일 내요을 읽기/쓰기를 위한 버퍼

	//fp_log = fopen( "./rtd.log", "a");
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


int el1000_sqlite3_delete( sqlite3 *pSQLite3, char *query )
{
	int rst;
	char *szErrMsg;

	printf("%s\n", query);
	rst = sqlite3_exec(
			pSQLite3,
			query,
			0,
			0,
			&szErrMsg);

	if( rst != SQLITE_OK )
	{
		printf("DELETE fail!! : %s\n", szErrMsg);
		//writeTextLog( szErrMsg );
		sqlite3_free( szErrMsg );
		SQLITE_SAFE_FREE( szErrMsg )
		return -1;
	}

	printf("delete rtrn = %d\n", rst);

	return 0;

}

Return_Selectdata el1000_sqlite3_select( sqlite3 *pSQLite3, char *query )
{
	int rst;
	int nrows 	= 0;
	int ncols	= 0;
	char *szErrMsg;
	char **past_result;

	Return_Selectdata data;


	printf("Select !!\n");

	//query = sqlite3_mprintf("select groupid, nodeid, temperature, humidity, lux, bat from tb_th1000_data where nodeid = 1 order by id desc limit 1");

	//rst = sqlite3_get_table(pSQLite3, query, &past_result, &nrows, &ncols, &szErrMsg);
	rst = sqlite3_get_table(pSQLite3, query, &data.past_result, &nrows, &ncols, &szErrMsg);

	if( rst != SQLITE_OK )
	{
		printf("%d\n", rst);
		printf("select fail!! : %s\n", szErrMsg);
		sqlite3_free( szErrMsg );
		SQLITE_SAFE_FREE( szErrMsg )

		sqlite3_free_table( data.past_result );
		SQLITE_SAFE_FREE( data.past_result )
	}

	data.size = (nrows * ncols + ncols);

	//sqlite3_free( query );
	//SQLITE_SAFE_FREE( query )


	//sqlite3_free_table( past_result );
	//SQLITE_SAFE_FREE( past_result )

	//return (nrows * ncols + ncols);
	return data;
}


int getNodeList( sqlite3 *pSQLite3, unsigned char* node)
{
	int rst;
	int nrows 	= 0;
	int ncols	= 0;
	int list;
	int i;
	char *szErrMsg;
	char **past_result;
	char *query;
	

	//query = sqlite3_mprintf("select nodeid from '%s' group by nodeid", TABLE_PATH);
	query = sqlite3_mprintf("select nodeid from tb_comm_status");

	rst = sqlite3_get_table(pSQLite3, query, &past_result, &nrows, &ncols, &szErrMsg);

	if( rst != SQLITE_OK )
	{
		printf("Error Code : %d\n", rst);
		printf("select fail!!(select nodeid) : %s\n", szErrMsg);
		sqlite3_free( szErrMsg );
		SQLITE_SAFE_FREE( szErrMsg )
	}

	list = (nrows * ncols + ncols);
	
	if( list > 0 )
	{
		for( i = 1; i < list; i++ )
		{
			node[i-1] = (unsigned char)atoi(past_result[i]);
			//printf("%d\n", node[i-1]);
		}
		list -= 1;
	}


	sqlite3_free( query );
	SQLITE_SAFE_FREE( query )

	sqlite3_free_table( past_result );
	SQLITE_SAFE_FREE( past_result )


	return list;
}

int getNodeInfo( sqlite3 *pSQLite3, char nodeid )
{
	int rst;
	int nrows 	= 0;
	int ncols	= 0;
	int count;
	char *szErrMsg;
	char **past_result;
	char *query;

	printf("nodeid = %d\n", nodeid);
	query = sqlite3_mprintf("select count(nodeid) from tb_th1000_data where nodeid = '%d'",
			nodeid		
			);

	printf("Select !!\n");

	rst = sqlite3_get_table(pSQLite3, query, &past_result, &nrows, &ncols, &szErrMsg);

	if( rst != SQLITE_OK )
	{
		printf("%d\n", rst);
		printf("select fail!! : %s\n", szErrMsg);
		sqlite3_free( szErrMsg );
		SQLITE_SAFE_FREE( szErrMsg )
	}

	printf("past_result[0]  = %s\n",  past_result[0]);
	printf("past_result[1]  = %s\n",  past_result[1]);

	printf("nrows = %d\n", nrows);
	printf("ncols = %d\n", ncols);

	count = atoi(past_result[1]);

	sqlite3_free( query );
	SQLITE_SAFE_FREE( query )

	sqlite3_free_table( past_result );
	SQLITE_SAFE_FREE( past_result )


	return count;
}



int RTDProcess( sqlite3 **pSQLite3, Return_RTDParser dispens )
{
	char *log_buf;
	char	*szErrMsg;
	int rst;


	//sqlite3_busy_handler( *pSQLite3, busy, NULL);


	if( dispens.alive )
	{
		log_buf = sqlite3_mprintf("UPDATE '%s' SET datetime = datetime('now','localtime'), nodeid = %d, temperature = '%s', humidity = '%s', lux = '%s', bat ='%s', seq = '%s', alive = %d WHERE nodeid = %d",
				TABLE_PATH,
				dispens.nodeid,
				dispens.temp,
				dispens.humi,
				dispens.lux,
				dispens.bat,
				dispens.seq,
				dispens.alive,
				dispens.nodeid
				);
	}
	else
	{
		log_buf = sqlite3_mprintf("UPDATE '%s' SET alive = %d WHERE nodeid = %d",
				TABLE_PATH,
				dispens.alive,
				dispens.nodeid
				);
	}

	rst = sqlite3_exec(
			*pSQLite3,
			log_buf,
			0,
			0,
			&szErrMsg);

	if( rst != SQLITE_OK )
	{
		printf("[%d] update fail!! : %s\n", dispens.nodeid, szErrMsg);
		sqlite3_free( szErrMsg );
		if( szErrMsg != NULL )
			szErrMsg = NULL;
	}

	sqlite3_free( log_buf );		// mem free
	if( log_buf != NULL )
		log_buf = NULL;


	return 0;
}

int InsertNode( sqlite3 **pSQLite3, char nodeid )
{
	char *log_buf;
	char	*szErrMsg;
	int rst;

	//sqlite3_busy_handler( *pSQLite3, busy, NULL);

	log_buf = sqlite3_mprintf("insert into '%s' (date, datetime, nodeid ) \
								values ( date('now'), datetime('now','localtime'), %d )",
			TABLE_PATH,
			nodeid
			);

	rst = sqlite3_exec(
			*pSQLite3,
			log_buf,
			0,
			0,
			&szErrMsg);

	if( rst != SQLITE_OK )
	{
		printf("[%d] insert fail!! : %s\n", nodeid, szErrMsg);
		sqlite3_free( szErrMsg );
		if( szErrMsg != NULL )
			szErrMsg = NULL;
	}

	sqlite3_free( log_buf );		// mem free
	if( log_buf != NULL )
		log_buf = NULL;


	return 0;
}

int InsertAustemNodeData( sqlite3 **pSQLite3, t_getNode	node )
{
	char *log_buf;
	char	*szErrMsg;
	int rst;

	/*
	struct timeval val;
	struct tm *ptm;

	gettimeofday(&val, NULL);
	ptm = localtime(&val.tv_sec);

	log_buf = sqlite3_mprintf("insert into '%s' ( datetime, nodeid, groupid, seq, tag, value) \
								values ( '%04d-%02d-%02d %02d:%02d:%02d.%03d', '%d','%d','%d','%d','%d')",
			TABLE_PATH,
			ptm->tm_year + 1900, ptm->tm_mon + 1, ptm->tm_mday,
			ptm->tm_hour, ptm->tm_min, ptm->tm_sec,
      		val.tv_usec/1000,
			node.nodeid,
			node.group,
			node.seq,
			node.port+51,
			node.cnt[node.port]
			);
			*/
	log_buf = sqlite3_mprintf("insert into '%s' ( datetime, nodeid, groupid, seq, tag, value) \
								values ( strftime('%%Y-%%m-%%d %%H:%%M:%%f', 'now','localtime'), '%d','%d','%d','%d','%d')",
			TABLE_PATH,
			node.nodeid,
			node.group,
			node.seq,
			node.port+51,
			node.cnt[node.port]
			);

	rst = sqlite3_exec(
			*pSQLite3,
			log_buf,
			0,
			0,
			&szErrMsg);

	if( rst != SQLITE_OK )
	{
		printf("[NodeID %d] insert fail!! : %s\n", node.nodeid, szErrMsg);
		//writeTextLog( szErrMsg );
		sqlite3_free( szErrMsg );
		if( szErrMsg != NULL )
			szErrMsg = NULL;
	}

	sqlite3_free( log_buf );		// mem free
	if( log_buf != NULL )
		log_buf = NULL;


	return 0;
}

int InsertAustemNodeStatusData( sqlite3 **pSQLite3, t_getNode	node )
{
	char *log_buf;
	char	*szErrMsg;
	int rst;

	log_buf = sqlite3_mprintf("insert into tb_comm_status ( datetime, nodeid, groupid, seq, port1, port2, port3, port4, port5, port6 ) \
								values ( strftime('%%Y-%%m-%%d %%H:%%M:%%f', 'now','localtime'), '%d','%d','%d','%d','%d','%d','%d','%d','%d')",
			node.nodeid,
			node.group,
			node.seq,
			node.cnt[0],
			node.cnt[1],
			node.cnt[2],
			node.cnt[3],
			node.cnt[4],
			node.cnt[5]
			);

	rst = sqlite3_exec(
			*pSQLite3,
			log_buf,
			0,
			0,
			&szErrMsg);

	if( rst != SQLITE_OK )
	{
		printf("[NodeID %d] insert fail!! : %s\n", node.nodeid, szErrMsg);
		//writeTextLog( szErrMsg );
		sqlite3_free( szErrMsg );
		if( szErrMsg != NULL )
			szErrMsg = NULL;
	}

	sqlite3_free( log_buf );		// mem free
	if( log_buf != NULL )
		log_buf = NULL;


	return 0;
}
int UpdateAustemNodeData( sqlite3 **pSQLite3, t_getNode	node )
{
	char *log_buf;
	char	*szErrMsg;
	int rst;


	log_buf = sqlite3_mprintf("UPDATE TB_COMM_STATUS SET datetime = strftime('%%Y-%%m-%%d %%H:%%M:%%f', 'now','localtime'), nodeid = %d, groupid = %d, seq = %d, port1 = %d, port2 = %d, port3 = %d, port4 = %d, port5 = %d, port6 = %d WHERE nodeid = %d",
			node.nodeid,
			node.group,
			node.seq,
			node.cnt[0],
			node.cnt[1],
			node.cnt[2],
			node.cnt[3],
			node.cnt[4],
			node.cnt[5],
			node.nodeid
			);

	rst = sqlite3_exec(
			*pSQLite3,
			log_buf,
			0,
			0,
			&szErrMsg);

	if( rst != SQLITE_OK )
	{
		printf("[NodeID %d] update fail!! : %s\n", node.nodeid, szErrMsg);
		//writeTextLog( szErrMsg );
		sqlite3_free( szErrMsg );
		if( szErrMsg != NULL )
			szErrMsg = NULL;
	}

	sqlite3_free( log_buf );		// mem free
	if( log_buf != NULL )
		log_buf = NULL;


	return 0;
}


/*
int InsertNode( sqlite3 **pSQLite3, Return_RTDParser dispens )
{
	char *log_buf;
	char	*szErrMsg;
	int rst;

	sqlite3_busy_handler( *pSQLite3, busy, NULL);

	log_buf = sqlite3_mprintf("insert into '%s' (datetime, nodeid, temperature, humidity, lux, bat ) \
								values (datetime('now','localtime'), %d,'%s','%s','%s','%s' )",
			TABLE_PATH,
			dispens.nodeid,
			dispens.temp,
			dispens.humi,
			dispens.lux,
			dispens.bat
			);

	rst = sqlite3_exec(
			*pSQLite3,
			log_buf,
			0,
			0,
			&szErrMsg);

	if( rst != SQLITE_OK )
	{
		printf("[%d] insert fail!! : %s\n", dispens.nodeid, szErrMsg);
		sqlite3_free( szErrMsg );
		if( szErrMsg != NULL )
			szErrMsg = NULL;
	}

	sqlite3_free( log_buf );		// mem free
	if( log_buf != NULL )
		log_buf = NULL;


	return 0;
}


int RTDProcess( sqlite3 **pSQLite3, Return_RTDParser dispens )
{
	char *log_buf;
	char	*szErrMsg;
	int rst;

	sqlite3_busy_handler( *pSQLite3, busy, NULL);

	log_buf = sqlite3_mprintf("insert into '%s' (datetime, groupid, nodeid, temperature, humidity, lux, bat ) \
								values (datetime('now','localtime'),'%s', %d,'%s','%s','%s','%s' )",
			TABLE_PATH,
			dispens.groupid,
			dispens.nodeid,
			dispens.temp,
			dispens.humi,
			dispens.lux,
			dispens.bat
			);

	rst = sqlite3_exec(
			*pSQLite3,
			log_buf,
			0,
			0,
			&szErrMsg);

	if( rst != SQLITE_OK )
	{
		printf("[%d] insert fail!! : %s\n", dispens.nodeid, szErrMsg);
		sqlite3_free( szErrMsg );
		if( szErrMsg != NULL )
			szErrMsg = NULL;
	}

	sqlite3_free( log_buf );		// mem free
	if( log_buf != NULL )
		log_buf = NULL;


	return 0;
}

*/


static int busy(void *handle, int nTry)
{
	//printf("%d th - busy handler is called\n", nTry);
	//usleep(10000);	// wait 10ms
	printf("kang[%d] busy handler is called\n", nTry);
	return 0;

	//return 10-nTry;
}
