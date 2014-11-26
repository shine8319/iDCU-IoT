#include <stdio.h> 
#include <string.h>
#include <time.h>


#include "../include/sqlite3.h"

#define SQLITE_SAFE_FREE(x)	if(x){ x = NULL; }
#define TABLE_PATH "tb_comm_log"

void writeTextLog( char* str )
{
	FILE    *fp_log;
	time_t 	curTime;
	struct tm *t;
	char     buff[1024]; // 파일 내요을 읽기/쓰기를 위한 버퍼

	//fp_log = fopen( "./rtd.log", "a");
	fp_log = fopen( "/work/comm/packet.log", "a");

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



int InsertDebugNodeData( sqlite3 **pSQLite3, unsigned char *buff)
{
	char *log_buf;
	char	*szErrMsg;
	int rst;

	log_buf = sqlite3_mprintf("insert into '%s' ( datetime, value) \
								values ( strftime('%%Y-%%m-%%d %%H:%%M:%%f', 'now','localtime'), '%02X%02X%02X%02X%02X%02X%02X%02X%02X%02X%02X%02X%02X%02X%02X%02X%02X%02X%02X%02X%02X%02X%02X%02X%02X%02X%02X%02X%02X%02X%02X%02X%02X%02X%02X%02X%02X%02X%02X%02X%02X%02X%02X%02X%02X%02X%02X%02X%02X%02X%02X%02X%02X%02X%02X%02X%02X%02X%02X%02X%02X%02X%02X%02X%02X%02X%02X%02X%02X%02X%02X%02X%02X%02X%02X%02X%02X%02X%02X%02X%02X%02X%02X%02X%02X')",
			TABLE_PATH,
			buff[0], buff[1], buff[2], buff[3], buff[4], buff[5], buff[6], buff[7], buff[8], buff[9],
			buff[10], buff[11], buff[12], buff[13], buff[14], buff[15], buff[16], buff[17], buff[18], buff[19],
			buff[20], buff[21], buff[22], buff[23], buff[24], buff[25], buff[26], buff[27], buff[28], buff[29],
			buff[30], buff[31], buff[32], buff[33], buff[34], buff[35], buff[36], buff[37], buff[38], buff[39],
			buff[40], buff[41], buff[42], buff[43], buff[44], buff[45], buff[46], buff[47], buff[48], buff[49],
			buff[50], buff[51], buff[52], buff[53], buff[54], buff[55], buff[56], buff[57], buff[58], buff[59],
			buff[60], buff[61], buff[62], buff[63], buff[64], buff[65], buff[66], buff[67], buff[68], buff[69],
			buff[70], buff[71], buff[72], buff[73], buff[74], buff[75], buff[76], buff[77], buff[78], buff[79],
			buff[80], buff[81], buff[82], buff[83], buff[84]
			);

	rst = sqlite3_exec(
			*pSQLite3,
			log_buf,
			0,
			0,
			&szErrMsg);

	if( rst != SQLITE_OK )
	{
		printf("Debug insert fail!! : %s\n", szErrMsg);
		writeTextLog( szErrMsg );
		sqlite3_free( szErrMsg );
		if( szErrMsg != NULL )
			szErrMsg = NULL;
	}

	sqlite3_free( log_buf );		// mem free
	if( log_buf != NULL )
		log_buf = NULL;


	return 0;
}

