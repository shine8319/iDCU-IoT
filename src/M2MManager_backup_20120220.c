#include <stdio.h> 
#include <stdlib.h> 
#include <string.h> 
#include <unistd.h>
#include <sys/ipc.h>
#include <sys/shm.h>
#include <signal.h>
#include <sys/socket.h>
#include <sys/types.h>
#include <arpa/inet.h>
#include <time.h>

#include "/usr/local/arm/expat/arm/include/expat.h" 
#include "/work/RnD/weather/include/sqlite3.h"
#include "/work/RnD/weather/include/PointManager.h"
#include "/work/RnD/weather/include/M2MManager.h"

#define DEBUG
#define DBPATH "/root/TH1000.sqlite3"


unsigned char user_quit = 0;

extern int el1000_sqlite3_open( char *path, sqlite3 **pSQLite3 );
extern int el1000_sqlite3_customize( sqlite3 **pSQLite3 );
extern int el1000_sqlite3_close( sqlite3 **pSQLite3 );
extern int el1000_sqlite3_transaction( sqlite3 **pSQLite3, char status );
extern Return_Selectdata el1000_sqlite3_select( sqlite3 *pSQLite3, char *query );
extern NODEINFO pointparser(const char *path);

void* Connect_Manager( char *ip, char *port );
int Socket_Manager( void* socket_fd, sqlite3 *pSQLite3 );
void quitsignal(int sig);
char getValueLength(int data);

typedef struct _PAC {
	char type;
	char length;
	char value[4];

} _PAC;

void quitsignal(int sig) 
{
	user_quit = 1;
}

int main(int argc, char **argv) { 

	int i;

	if (argc < 4) {
		printf ("%d ",argc);
		fprintf(stderr, "Usage: %s <Server IP> <Port> <DB Path>\n", argv[0]);
		//exit(2);
		return -1;
	}

	(void)signal(SIGINT, quitsignal);
	printf("Press <ctrl+c> to user_quit. \n");

	/******************** DB connect ***********************/
	int rc;
	sqlite3 *pSQLite3;
	//char	*szErrMsg;

	// DB Open
	rc = el1000_sqlite3_open( DBPATH, &pSQLite3 );
	if( rc != 0 )
	{
		return -1;
	}
	else
		printf("%s OPEN!!\n", DBPATH);

	// DB Customize
	rc = el1000_sqlite3_customize( &pSQLite3 );
	if( rc != 0 )
		return -1;
	/*******************************************************/


	/***************** Server Connect **********************/
	void* socket_fd;
	while( !user_quit ) 
	{

		socket_fd = Connect_Manager( argv[1], argv[2] );

		Socket_Manager( socket_fd, pSQLite3 );

	}


	/*******************************************************/
	el1000_sqlite3_close( &pSQLite3 );

	printf("[M2MManager] end of main loop \n");
	return 0; 
} 



void* Connect_Manager( char *ip, char *port ) 
{
	struct sockaddr_in servaddr; //server addr
	//int bufsize = 100000;
	//int rn = sizeof(int);
	int server_sock;


	printf("Start Socket !!\n");


//	while( !user_quit ) {

		/* create socket */
		server_sock = socket(PF_INET, SOCK_STREAM, 0);

		// add by hyungoo.kang
		// solution ( bind error : Address already in use )
		/*
		if( setsockopt( server_sock, SOL_SOCKET, SO_REUSEADDR, (void*)&bufsize, (socklen_t)rn) < 0 ) {
			perror("Error setsockopt()");
			return;
		}
		*/

		memset(&servaddr, 0, sizeof(servaddr));
		servaddr.sin_family = AF_INET;
		servaddr.sin_addr.s_addr = inet_addr(ip);
		servaddr.sin_port = htons(atoi(port));

		if( connect( server_sock, (struct sockaddr*)&servaddr, sizeof(servaddr)) == -1 )
		{
			printf("connnect error\n");
		}


		printf("New Connection,  Server IP : %s (%d)\n",	inet_ntoa(servaddr.sin_addr), server_sock );
		//Socket_Manager( (void*)server_sock, &pSQLite3 );

//	}

	//close( server_sock );

	return (void*)server_sock;


}



#define SQLITE_SAFE_FREE(x)	if(x){ x = NULL; }
int Socket_Manager( void* socket_fd, sqlite3 *pSQLite3 ) {

	int sock = (int)socket_fd;
	int select_size;
	char *szErrMsg;
	int rst;
	int nrows 	= 0;
	int ncols	= 0;
	int i;
	int atoi_cnt;

	int selectdata[1024];
	_PAC pac[5];
int send_size;
	enum {
		TEMP = 101,
		HUMI = 102,
		LUX = 103,
		BAT = 104,
	};
#define _1byte 0
#define _2byte 1
#define _3byte 2
#define _4byte 3
char parsinglength;
int parsingvalue;
char send_buff[1024];
int buff_cnt = 1;

	NODEINFO tag = pointparser("usn_node_info.xml");

	printf("tag.getPointSize = %d\n", tag.getPointSize);


	while( !user_quit ) 
	{


	char *query;
	char **past_result;
	Return_Selectdata data;

	int tagcnt;

	for( tagcnt = 0; tagcnt < tag.getPointSize; tagcnt++)
	{
		query = sqlite3_mprintf("select groupid, nodeid, temperature, humidity, lux, bat from tb_th1000_data where nodeid = '%d' order by id desc limit 1",
								tag.id[tagcnt]		
								);
		data = el1000_sqlite3_select( pSQLite3, query );
		//past_result = el1000_sqlite3_select( pSQLite3, query );
		//printf("atoi(past_result[5] = %d\n", atoi(past_result[5]));
		printf("select size = %d\n", data.size );

		atoi_cnt = 0;
		if( data.size > 0 )
		{

			for( i = 7; i < 12; i++ )
			{
				selectdata[atoi_cnt++] =  atoi(data.past_result[i]);
			}


			/*
			for( i = 0; i < atoi_cnt; i++ )
			{
				printf("%d | ", selectdata[i]);
			}
			printf("\n");
			*/



			for( i = 0; i < atoi_cnt; i++ )
			{

				if( i == 0 )
					pac[i].type = 0x0C;
				else if( i == 1 )
					pac[i].type = TEMP << 1;
				else if( i == 2 )
					pac[i].type = HUMI << 1;
				else if( i == 3 )
					pac[i].type = LUX << 1;
				else if( i == 4 )
					pac[i].type = BAT << 1;

				pac[i].type |= 0x1;
				send_buff[buff_cnt++] = pac[i].type;

				if( i == 0 )
				{
					parsinglength = 8;
					pac[i].length = parsinglength << 1;
				}
				else
				{
					parsinglength = getValueLength(selectdata[i]);
					pac[i].length = parsinglength << 1;
				}

				pac[i].length|= 0x1;
				send_buff[buff_cnt++] = pac[i].length;

				parsingvalue = selectdata[i];
				parsingvalue = htons(parsingvalue);
				//parsingvalue <<= 1;
				memcpy( pac[i].value, &parsingvalue, parsinglength );
				//pac[i].value = parsingvalue << 1;
				if( parsinglength == 1 )
				{
					//pac[i].value[_1byte] |= 0x1;
					//send_buff[buff_cnt++] = pac[i].value[_1byte];
					send_buff[buff_cnt++] = (char)selectdata[i];
					printf("pac[%d].value[_1byte] = %d, %d\n", i,pac[i].value[_1byte], (selectdata[i]>>24));
					printf("pac[%d].value[_2byte] = %d, %d\n", i,pac[i].value[_2byte], (selectdata[i]>>16));
					printf("pac[%d].value[_3byte] = %d, %d\n", i,pac[i].value[_3byte], (selectdata[i]>>8));
					printf("pac[%d].value[_4byte] = %d, %d\n", i,pac[i].value[_4byte], (selectdata[i]>>0));
				}
				else if( parsinglength == 2 )
				{
					//pac[i].value[_1byte] |= 0x1;
					send_buff[buff_cnt++] = pac[i].value[_1byte];
					//pac[i].value[_2byte] <<= 1;
					//pac[i].value[_2byte] |= 0x0;
					send_buff[buff_cnt++] = pac[i].value[_2byte];
				}
				else if( parsinglength == 3 )
				{
					//pac[i].value[_1byte] |= 0x1;
					send_buff[buff_cnt++] = pac[i].value[_1byte];
					//pac[i].value[_2byte] <<= 1;
					//pac[i].value[_2byte] |= 0x0;
					send_buff[buff_cnt++] = pac[i].value[_2byte];
					//pac[i].value[_3byte] <<= 1;
					//pac[i].value[_3byte] |= 0x0;
					send_buff[buff_cnt++] = pac[i].value[_3byte];
				}
				else if( parsinglength == 4 )
				{
					//pac[i].value[_1byte] |= 0x1;
					send_buff[buff_cnt++] = pac[i].value[_1byte];
					//pac[i].value[_2byte] <<= 1;
					//pac[i].value[_2byte] |= 0x0;
					send_buff[buff_cnt++] = pac[i].value[_2byte];
					//pac[i].value[_3byte] <<= 1;
					//pac[i].value[_3byte] |= 0x0;
					send_buff[buff_cnt++] = pac[i].value[_3byte];
					//pac[i].value[_4byte] <<= 1;
					//pac[i].value[_4byte] |= 0x0;
					send_buff[buff_cnt++] = pac[i].value[_4byte];
				}
				else if( parsinglength == 8 )
				{

					send_buff[buff_cnt++] = 0x00;
					send_buff[buff_cnt++] = 0xE0;
					send_buff[buff_cnt++] = 0x4A;
					send_buff[buff_cnt++] = 0xBC;
					send_buff[buff_cnt++] = 0x15;
					send_buff[buff_cnt++] = 0xE7;

					send_buff[buff_cnt++] = 0x0C;
					send_buff[buff_cnt++] = (char)(parsingvalue >> 8);
					printf("parsingvalue >> 24 = %d\n",  (char)(parsingvalue >> 24));
					printf("parsingvalue >> 16 = %d\n",  (char)(parsingvalue >> 16));
					printf("parsingvalue >> 8 = %d\n",  (char)(parsingvalue >> 8));
					printf("parsingvalue >> 0 = %d\n",  (char)(parsingvalue >> 0));

				}


				printf("%d | ", selectdata[i]);
			}
			printf("\n");


			send_buff[0] = buff_cnt-1;
			send_buff[0] <<= 1;
			send_buff[0] |= 0x1;
			send_size = send( sock, &send_buff, buff_cnt, 0 );
			if( send_size == -1 )
				user_quit = 1;
			printf("send data size : %d\n", send_size );
			buff_cnt = 0;



		}

		sqlite3_free( query );
		SQLITE_SAFE_FREE( query )
		sqlite3_free_table( data.past_result );
		SQLITE_SAFE_FREE( data.past_result )
	}

	

		sleep(10);

	}

	return 0;

}


char getValueLength(int data)
{

	char size;

	if( data <= 255)
		size = 1;
	else if( 255 < data && data <= 65535 )
		size = 2;
	else if( 65535 < data && data <= 16777215 )
		size = 3;
	else if( data > 16777215 )
		size = 4;

	/*
	if( data < 128 )
		size = 1;
	else if( 128 <= data && data <= 32767 )
		size = 2;
	else if( 32768 <= data && data <= 8388607 )
		size = 3;
	else if( data > 8388607 )
		size = 4;
		*/

	return size;
}

		

		
