#include <stdio.h> 
#include <stdlib.h> 
#include <string.h> 
#include <unistd.h>
#include <sys/ipc.h>
#include <sys/msg.h>
#include <sys/shm.h>
#include <signal.h>
#include <sys/socket.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <arpa/inet.h>
#include <fcntl.h>
#include <errno.h>
#include <time.h>
#include <termio.h>
#include <sys/wait.h>

#include "./include/iDCU.h"
#include "./include/sqlite3.h"
#include "./include/M2MManager.h"

#define LOGPATH "/work/log/USNManager"
#define DBPATH "/work/db/dabom-device"
#define SQLITE_SAFE_FREE(x)	if(x){ x = NULL; }

unsigned char main_quit = 0;

extern int el1000_sqlite3_open( char *path, sqlite3 **pSQLite3 );
extern int el1000_sqlite3_customize( sqlite3 **pSQLite3 );
extern int el1000_sqlite3_close( sqlite3 **pSQLite3 );
extern int el1000_sqlite3_transaction( sqlite3 **pSQLite3, char status );
extern int el1000_sqlite3_update( sqlite3 **pSQLite3, char *query );
extern Return_Selectdata el1000_sqlite3_select( sqlite3 *pSQLite3, char *query );
extern int M2MProtocol( void* socket_fd, sqlite3 *pSQLite3, QUERYTIME time, char type, int keepdata);
extern Return_Selectdata getNodeInfo( sqlite3 *pSQLite3, char *query );
extern int el1000_sqlite3_delete( sqlite3 *pSQLite3, char *query );

int Pack_Manager( unsigned char* buffer, int len );
int getNodeData( QUERYTIME time );
int getNodeStatusData( QUERYTIME time );
static int busy(void *handle, int nTry);
unsigned char _getCheckSum( int len );

int commandGetNodeInfo( );
int commandDeleteNode( unsigned char nodeid );
int commandNodeConfig( unsigned char* buffer, int len );
int commandM2MConfig( unsigned char* buffer, int len );
int nodeInfoupdate( unsigned char nodeid, unsigned char setNodeid, unsigned char setGroup);

typedef struct _PAC {
	char type;
	char length;
	char value[4];

} _PAC;


int m2mid;
int eventid;

AustemStatusSendData austemStatusSend[256];
AustemSendData austemSend[2048];
unsigned char tlvSendBuffer[20480];



sqlite3 *pSQLite3;
int wifi = 0;

int server_sock; 
int client_sock; 


int main(int argc, char **argv) { 

	int i;
	pthread_t p_thread;
	int thr_id;
	int status;

	int rc;

	void *socket_fd;
	//int socket_fd;
	struct sockaddr_in servaddr; //server addr

	// wifi
	struct termios tio, old_tio;
	int ret;
	//sleep(2);

	/******************** DB connect ***********************/
	//char	*szErrMsg;

	// DB Open
	rc = el1000_sqlite3_open( DBPATH, &pSQLite3 );
	if( rc != 0 )
	{
		//writeLog( "error DB Open" );
		writeLogV2(LOGPATH, "[USNManager]", "error DB Open\n");
		return -1;
	}
	else
	{
		writeLogV2(LOGPATH, "[USNManager]", "%s OPEN!!\n", DBPATH);
		printf("%s OPEN!!\n", DBPATH);
	}


	//sqlite3_busy_handler( pSQLite3, busy, NULL);
	sqlite3_busy_timeout( pSQLite3, 1000);
	//sqlite3_busy_timeout( pSQLite3, 5000);

	// DB Customize
	rc = el1000_sqlite3_customize( &pSQLite3 );
	if( rc != 0 )
	{
		//writeLog( "error DB Customize" );
		writeLogV2(LOGPATH, "[USNManager]", "error DB Customize\n");
		return -1;
	}


	/*******************************************************/
	// wifi
	/*
	wifi = open( "/dev/ttyUSB0", O_RDWR, 0);
	if(wifi == -1) {
		perror("open()\n");
		writeLog( "error open /dev/ttyUSB0" );
		return -1;
	}
	memset(&tio, 0, sizeof(tio)); 
	tio.c_iflag = IGNPAR; 
	//tio.c_cflag = B57600 | HUPCL | CS8 | CLOCAL | CREAD; 
	tio.c_cflag = B115200 | HUPCL | CS8 | CLOCAL | CREAD; 
	tio.c_cc[VTIME] = 1;
	tio.c_cc[VMIN] = 0;
	tcgetattr(wifi, &old_tio);
	tcsetattr(wifi, TCSANOW, &tio);
	*/

	/***************** Server Connect **********************/
	if( -1 == ( m2mid = msgget( (key_t)2222, IPC_CREAT | 0666)))
	{
		//writeLog( "error msgget() m2mid" );
		writeLogV2(LOGPATH, "[USNManager]", "error msgget() m2mid\n");
		//perror( "msgget() 실패");
		return -1;
	}

	if( -1 == ( eventid = msgget( (key_t)3333, IPC_CREAT | 0666)))
	{
		//writeLog( "error msgget() eventid" );
		writeLogV2(LOGPATH, "[USNManager]", "error msgget() eventid\n");
		//perror( "msgget() 실패");
		return -1;
	}


	Connect_Manager();

	/*
	while(1) 
	{

		ReadMsgSize = read(wifi, DataBuf, BUFFER_SIZE); 
		if( ReadMsgSize > 0 )
		{
			if( receiveSize >= BUFFER_SIZE )
				continue;

			memcpy( receiveBuffer+receiveSize, DataBuf, ReadMsgSize );
			receiveSize += ReadMsgSize;

			parsingSize = ParsingReceiveValue(receiveBuffer, receiveSize, remainder, parsingSize);
			memset( receiveBuffer, 0 , sizeof(BUFFER_SIZE) );
			receiveSize = 0;
			memcpy( receiveBuffer, remainder, parsingSize );
			receiveSize = parsingSize;

			memset( remainder, 0 , sizeof(BUFFER_SIZE) );
		}			
		else
		{
			//printf("Serial timeout %d\n", ReadMsgSize);
			sleep(1);
		}

	}
	*/


	/*******************************************************/
	el1000_sqlite3_close( &pSQLite3 );

	printf("[M2MManager] end of main loop \n");
	return 0; 
} 

int Socket_Manager( void ) {

	fd_set control_msg_readset;
	struct timeval control_msg_tv;
	unsigned char DataBuf[BUFFER_SIZE];
	int ReadMsgSize;

	unsigned char receiveBuffer[BUFFER_SIZE];
	int receiveSize = 0;
	unsigned char remainder[BUFFER_SIZE];
	int parsingSize = 0;


	int rtrn;
	int i;
	int nd;

	FD_ZERO(&control_msg_readset);
	printf("Receive Ready!!!\n");

	while( 1 ) 
	{

		FD_SET(client_sock, &control_msg_readset);
		control_msg_tv.tv_sec = 15;
		//control_msg_tv.tv_usec = 10000;
		control_msg_tv.tv_usec = 5000000;	// timeout check 5 second

		// 리턴값 -1은 오류발생, 0은 타임아웃, 0보다 크면 변경된 파일 디스크립터수
		nd = select( client_sock+1, &control_msg_readset, NULL, NULL, &control_msg_tv );		
		if( nd > 0 ) 
		{

			memset( DataBuf, 0, sizeof(DataBuf) );
			ReadMsgSize = recv( client_sock, &DataBuf, BUFFER_SIZE, MSG_DONTWAIT);
			if( ReadMsgSize > 0 ) 
			{
				for( i = 0; i < ReadMsgSize; i++ ) {
					printf("%-3.2x", DataBuf[i]);
				}
				printf("\n");
				printf("recv data size : %d\n", ReadMsgSize);

				if( ReadMsgSize >= BUFFER_SIZE )
					continue;

				memcpy( receiveBuffer+receiveSize, DataBuf, ReadMsgSize );
				receiveSize += ReadMsgSize;

				parsingSize = ParsingReceiveValue(receiveBuffer, receiveSize, remainder, parsingSize);
				memset( receiveBuffer, 0 , sizeof(BUFFER_SIZE) );
				receiveSize = 0;
				memcpy( receiveBuffer, remainder, parsingSize );
				receiveSize = parsingSize;

				memset( remainder, 0 , sizeof(BUFFER_SIZE) );

			} 
			else {
				sleep(1);
				printf("receive None\n");
				break;
			}
			ReadMsgSize = 0;

		} 
		else if( nd == 0 ) 
		{
			printf("timeout\n");
			//shutdown( client_sock, SHUT_WR );
			break;
		}
		else if( nd == -1 ) 
		{
			printf("error...................\n");
			//shutdown( client_sock, SHUT_WR );
			break;
		}
		nd = 0;

	}	// end of while

	printf("Disconnection client....\n");

	close( client_sock );
	return 0;

}


int Connect_Manager() {

	struct sockaddr_in servaddr; //server addr
	struct sockaddr_in clientaddr; //client addr
	unsigned short port = 50007;
	int client_addr_size;
	int bufsize = 100000;
	int rn = sizeof(int);
	int flags;
	pid_t pid;

	printf("Start Socket Server!!\n");

	while( 1 ) {

		/* create socket */
		server_sock = socket(PF_INET, SOCK_STREAM, 0);

		// add by hyungoo.kang
		// solution ( bind error : Address already in use )
		if( setsockopt( server_sock, SOL_SOCKET, SO_REUSEADDR, (void*)&bufsize, (socklen_t)rn) < 0 ) {
			perror("Error setsockopt()");
			return;
		}

		memset(&servaddr, 0, sizeof(servaddr));
		servaddr.sin_family = AF_INET;
		servaddr.sin_addr.s_addr = htonl(INADDR_ANY);
		servaddr.sin_port = htons(port);

		if (bind(server_sock, (struct sockaddr*)&servaddr, sizeof(servaddr)) < 0) {
			printf("server socket bind error\n");
			return -1;
		}

		listen(server_sock, 5);

		client_addr_size = sizeof(clientaddr);
		client_sock = accept(server_sock, (struct sockaddr*)&clientaddr, &client_addr_size);


		if((flags = fcntl(client_sock, F_GETFL, 0)) == -1 ) {
			perror("fcntl");
			return;
		}
		if(fcntl(client_sock, F_GETFL, flags | O_NONBLOCK) == -1) {
			perror("fcntl");
			return;
		}

		close( server_sock );

		printf("New Connection, Client IP : %s (%d)\n",	inet_ntoa(clientaddr.sin_addr), client_sock);
		Socket_Manager();
		printf("TCP : Client No Signal. Connection Try Again....\n");

	}

	close( client_sock );

}

int ParsingReceiveValue(unsigned char* cvalue, int len, unsigned char* remainder, int remainSize )
{

	unsigned char setBuffer[BUFFER_SIZE];
	int i;
	int offset;

	for( i = 0; i < len; i++ )
	{

		if( cvalue[i] == 0xFE )
		{

			//if( len >= i + PACKET_SIZE && ( cvalue[i + (PACKET_SIZE-1)] == 0xFF ) )
			if( len >= i + cvalue[i+1] && ( cvalue[i + (cvalue[i+1]+2)] == 0xFF ) )
			{
				// 데이터 수신정상
				memcpy( setBuffer, cvalue+i, cvalue[i+1]+3 );

				/*
				for( offset = 0; offset < cvalue[i+1]+3; offset++ )
					printf("%02X ", setBuffer[offset]);
				printf("\n");
				*/


				i += (cvalue[i+1]+2);

				remainSize = i+1;
				Pack_Manager( setBuffer, i+1 );

			}

		}

	}

	remainSize = len - remainSize;
	memcpy( remainder, cvalue+(len-(remainSize)), remainSize );


	return remainSize;
}

int AustemTimeSync( QUERYTIME time )
{

		char settime[15]; 
		int pid;
		int status;
		int state;
		pid_t child;

		int _2pid;
		int _2state;
		pid_t _2child;
		int rtrn;


		sprintf(settime, "%02d%02d%02d%02d20%02d.%02d", 
				time.month,
				time.day,
				time.hour,
				time.minute,
				time.year,
				time.sec );

		if( time.year < 12 || time.year > 99 
				|| time.month < 1 || time.month > 12 
				|| time.day < 1 || time.day > 31 
				|| time.hour > 24 
				|| time.minute > 59
				|| time.sec > 59 )
		{
			printf(" Out of Set time \n ");
			return -1;
		}
			
		printf("request set time = %s\n", settime);
		pid = fork();
		if( pid < 0 )
		{
			perror("fork error : ");
			rtrn = -1;
		}
		if( pid == 0 )
		{
			printf("in child\n");
			execl("/bin/date", "/bin/date", settime, NULL);
			//execl("/sbin/hwclock -w", "/bin/date", settime, NULL);
			exit(0);
		}
		else
		{
			do {
				child = waitpid(-1, &state, WNOHANG);
			} while(child == 0);
			printf("Parent : wait(%d)\n", pid);
			printf("Child process id = %d, return value = %d \n", child, WEXITSTATUS(state));
			rtrn = 0;
		}

		_2pid = fork();
		if( _2pid < 0 )
		{
			perror("fork error : ");
			rtrn = -1;
		}
		if( _2pid == 0 )
		{
			printf("in 2child\n");
			execl("/work/smart/hwclock", "/work/smart/hwclock", "-w", NULL);
			exit(0);
		}
		else
		{
			do {
				_2child = waitpid(-1, &_2state, WNOHANG);
			} while(_2child == 0);
			printf("Parent : wait(%d)\n", _2pid);
			printf("Child process id = %d, return value = %d \n", _2child, WEXITSTATUS(_2state));
			rtrn = 0;
		}



		return rtrn;

}
int Pack_Manager( unsigned char* buffer, int len ) 
{
	int offset = 0;
	int cntOffset = 0;

	int i;
	int rtrn;
	unsigned char type,nodeid;

	QUERYTIME callTime;

	time_t	tm_st;
	time_t	tm_nd;
	int	tm_year, tm_month;
	int	tm_day, tm_hour, tm_min, tm_sec;
	double d_diff;
	struct tm t;
	char syncError;

	time_t 	curTime;
	struct tm *ct;





	type = buffer[2];
	nodeid = buffer[4];

	/*

	   if( buffer[7] == 'T' )
	   {
	   printf("Time sycn\n");

	//M2MProtocol( socket_fd, pSQLite3, callTime, recv_buff[7], (int)dev.keepdata[0] );
	}
	else
	*/
	//printf(" call Type %d\n", type);
	switch( type )
	{
		case TIMESYNC:

			callTime.year = buffer[5];
			callTime.month = buffer[6];
			callTime.day = buffer[7];
			callTime.hour = buffer[8];
			callTime.minute = buffer[9];
			callTime.sec = buffer[10];

			rtrn = AustemTimeSync( callTime );
			//printf("AustemTimeSync Return Value %d \n", rtrn);
			if( rtrn == 0 )
			{
				// OK 
				syncError = 0;
			}
			else
			{
				// error
				syncError = 1;
			}	

			tlvSendBuffer[0] = 0xFE;
			tlvSendBuffer[1] = 0x00;
			tlvSendBuffer[2] = 0x0D;
			tlvSendBuffer[3] = 0x0A;
			tlvSendBuffer[4] = 0x00;
			tlvSendBuffer[5] = 0x0A;
			tlvSendBuffer[6] = 0x02;
			tlvSendBuffer[7] = 0x07;
			tlvSendBuffer[8] = 0x54;
			memcpy( tlvSendBuffer+9, &callTime, 6 );
			tlvSendBuffer[15] = syncError;
			tlvSendBuffer[16] = 0xFF;

			//rtrn = write(wifi, tlvSendBuffer, 17);
			rtrn = write(client_sock, tlvSendBuffer, 17);
			//printf("Send rtrn = %d\n", rtrn);
			memset( tlvSendBuffer, 0, 20480 ); 
			break;

		case GETDATA:
			/*
			printf("Call Time %02d/%02d/%02d-%02d:%02d:%02d \n", 
					buffer[4],
					buffer[5],
					buffer[6],
					buffer[7],
					buffer[8],
					buffer[9]
				  );
				  */

			callTime.year = buffer[4];
			callTime.month = buffer[5];
			callTime.day = buffer[6];
			callTime.hour = buffer[7];
			callTime.minute = buffer[8];
			callTime.sec = buffer[9];

			t.tm_year = 2000 + buffer[4] - 1900;
			t.tm_mon = buffer[5] -1;
			t.tm_mday = buffer[6];
			t.tm_hour = buffer[7];
			t.tm_min = buffer[8];
			t.tm_sec = buffer[9];
			//printf("Year %d\n", t.tm_year);

			tm_st = mktime( &t );
			time( &tm_nd );

			d_diff = difftime( tm_nd, tm_st );
			//printf(" d_diff = %d\n", d_diff );
			//tm_year   = d_diff / ( ( 60 *60 * 24) * 365 );

			/*
			tm_day   = d_diff / ( 60 *60 * 24);
			d_diff   = d_diff - ( tm_day *60 *60 *24);
			printf(" d_diff = %d\n", d_diff );

			tm_hour  = d_diff / ( 60 *60);
			d_diff   = d_diff - ( tm_hour *60 *60);
			printf(" d_diff = %d\n", d_diff );

			tm_min   = d_diff / 60;
			d_diff   = d_diff - ( tm_min *60);
			printf(" d_diff = %d\n", d_diff );

			tm_sec   = d_diff;

			printf( "%d year %d일 %d시 %d분 %d초 지났음\n", 
					tm_year,
					tm_day, tm_hour, tm_min, tm_sec);
					*/
			curTime = time(NULL);
			ct = localtime(&curTime);
			//printf("%d > %d ( diff %d )\n", (2000+buffer[4]), (ct->tm_year + 1900), d_diff );
	
	
			//if( (2000+buffer[4]) > (ct->tm_year + 1900) || d_diff < 0 )
			if( buffer[4] == 99 || d_diff < 0 )
				getNodeStatusData( callTime );
			else
				getNodeData( callTime );
			
			break;

		case GETNODELIST:
			commandGetNodeInfo();
			break;
		case DELETENODE:
			commandM2MConfig( buffer, len );
			commandDeleteNode( nodeid );
			break;
		case CHANGENODEINFO:
			commandNodeConfig( buffer, len );
			commandM2MConfig( buffer, len );
			nodeInfoupdate( buffer[4], buffer[5], buffer[6] );
			break;
		case REBOOTNODE:
			if( nodeid != 0 )
				commandNodeConfig( buffer, len );
			else
			{
				commandNodeConfig( buffer, len );
				commandM2MConfig( buffer, len );
			}

			break;
	}

	return 0;

}

int commandM2MConfig( unsigned char* buffer, int len )
{

	t_data data;

	memset( &data, 0, sizeof( t_data ) );

	data.data_type = 1;   // data_type 는 1
	memcpy( data.data_buff, buffer, len );
	data.data_num = len; 

	if ( -1 == msgsnd( eventid, &data, sizeof( t_data) - sizeof( long), IPC_NOWAIT))
	{
		perror( "msgsnd() 실패");
		return -1;
	}

	return 0;
}

int commandNodeConfig( unsigned char* buffer, int len )
{

	t_data data;

	memset( &data, 0, sizeof( t_data ) );

	data.data_type = 1;   // data_type 는 1
	memcpy( data.data_buff, buffer, len );
	data.data_num = len; 

	//sprintf( data.data_buff, "type=%d, ndx=%d, USN Return Msg", data.data_type, data.data_num);
	if ( -1 == msgsnd( m2mid, &data, sizeof( t_data) - sizeof( long), IPC_NOWAIT))
	{
		perror( "msgsnd() 실패");
		//exit( 1);
		return -1;
	}

	return 0;
}

int commandDeleteNode( unsigned char nodeid )
{
	char* delQuery;

	delQuery = sqlite3_mprintf("delete from tb_comm_status where nodeid = '%d'",
			nodeid);

	el1000_sqlite3_delete( pSQLite3, delQuery );

	sqlite3_free( delQuery );
	SQLITE_SAFE_FREE( delQuery )

		return 0;
}


int nodeInfoupdate( unsigned char nodeid, unsigned char setNodeid, unsigned char setGroup)
{
	char* query;
	//query = sqlite3_mprintf("UPDATE TB_COMM_STATUS SET nodeid = %d, groupid = %d WHERE nodeid = %d",
	query = sqlite3_mprintf("DELETE FROM TB_COMM_STATUS WHERE nodeid = %d",
			nodeid );

	el1000_sqlite3_update( &pSQLite3, query );
	return 0;
}

int commandGetNodeInfo( )
{
	int vLength = 0;
	int totalLength = 0;
	int	rowCnt = 0;
	//int i, selectOffset = 2;
	int i, selectOffset = 3;	// edit 2013.11.15 add send datatime
	char yearConvert[3];
	char monthConvert[3];
	char dayConvert[3];
	char hourConvert[3];
	char minConvert[3];
	char secConvert[3];
	
	char* query;
	unsigned char sendBuffer[1024];
	Return_Selectdata selectData;
	RTRNNODEGROUP send[64];
	memset( send, 0, sizeof( RTRNNODEGROUP )*64);
	memset( sendBuffer, 0, 1024 );

	memset( yearConvert, 0, 3);
	memset( monthConvert, 0, 3);
	memset( dayConvert, 0, 3);
	memset( hourConvert, 0, 3);
	memset( minConvert, 0, 3);
	memset( secConvert, 0, 3);

	query = sqlite3_mprintf("select nodeid, groupid, strftime('%%Y%%m%%d%%H%%M%%S', datetime) from tb_comm_status order by nodeid asc");
	//query = sqlite3_mprintf("select nodeid, groupid from tb_comm_status order by nodeid asc");

	selectData = getNodeInfo( pSQLite3, query );
	//printf("select size = %d\n", selectData.size );
	sendBuffer[0] = 0xFE;


	if( selectData.size > 0 )
	{
		//for( i = 2; i <= selectData.size/2; i++ )
		for( i = 0; i < (selectData.size-3)/3; i++ )	// edit 2013.11.15
		{
			//printf("%s\n", selectData.past_result[i]);
			send[i].nodeid = atoi(selectData.past_result[selectOffset++]);
			send[i].group = atoi(selectData.past_result[selectOffset++]);
			yearConvert[0] = selectData.past_result[selectOffset][2];
			yearConvert[1] = selectData.past_result[selectOffset][3];
			send[i].time.year = atoi(yearConvert);
			monthConvert[0] = selectData.past_result[selectOffset][4];
			monthConvert[1] = selectData.past_result[selectOffset][5];
			send[i].time.month = atoi(monthConvert);
			dayConvert[0] = selectData.past_result[selectOffset][6];
			dayConvert[1] = selectData.past_result[selectOffset][7];
			send[i].time.day = atoi(dayConvert);
			hourConvert[0] = selectData.past_result[selectOffset][8];
			hourConvert[1] = selectData.past_result[selectOffset][9];
			send[i].time.hour = atoi(hourConvert);
			minConvert[0] = selectData.past_result[selectOffset][10];
			minConvert[1] = selectData.past_result[selectOffset][11];
			send[i].time.min = atoi(minConvert);
			secConvert[0] = selectData.past_result[selectOffset][12];
			secConvert[1] = selectData.past_result[selectOffset][13];
			send[i].time.sec = atoi(secConvert);
			selectOffset++;


			printf("nodeid %d, group %d\n", send[i].nodeid, send[i].group );
		}

		//rowCnt =  (selectData.size-2)/2;
		rowCnt =  (selectData.size-3)/3;
		vLength = sizeof(RTRNNODEGROUP)*rowCnt;	// add 20130820 time 6 byte
		//printf("vLength = %d\n", vLength );
		totalLength = vLength + 3;	// type 1byte, length 2byte
		//printf("totalLength = %d\n", totalLength );

		sendBuffer[1] = totalLength >> 8;	// total length;
		sendBuffer[2] = totalLength;	// total length;
		sendBuffer[3] = 0x05;
		sendBuffer[4] = vLength >> 8;
		sendBuffer[5] = vLength;
		memcpy( sendBuffer+6, send, vLength );
		sendBuffer[totalLength+3] = 0xFF;

		//int rtrn = write(wifi, sendBuffer, totalLength+4);
		int rtrn = write(client_sock, sendBuffer, totalLength+4);
		//printf("Send rtrn = %d\n", rtrn);
	}



	sqlite3_free( query );
	SQLITE_SAFE_FREE( query )
		sqlite3_free_table( selectData.past_result );
	SQLITE_SAFE_FREE( selectData.past_result )
}


int getNodeStatusData( QUERYTIME time ) {

	int i;
	int vLength = 0;
	int totalLength = 0;
	char *query;
	char *szErrMsg;
	int	rowSize = 9;	// edit 2014.05.13
	int	rowCnt = 0;
	unsigned int byteOrder = 0;

	char yearConvert[3];
	char monthConvert[3];
	char dayConvert[3];
	char hourConvert[3];
	char minConvert[3];
	char secConvert[3];
	char mSecConvert[4];

	memset( yearConvert, 0, 3);
	memset( monthConvert, 0, 3);
	memset( dayConvert, 0, 3);
	memset( hourConvert, 0, 3);
	memset( minConvert, 0, 3);
	memset( secConvert, 0, 3);
	memset( mSecConvert, 0, 4);
	

	Return_Selectdata data;

	//query = sqlite3_mprintf("select nodeid, 6, port1, port2, port3, port4, port5, port6 from tb_comm_status");
	query = sqlite3_mprintf("select nodeid, 6, port1, port2, port3, port4, port5, port6, strftime('%%Y%%m%%d%%H%%M%%f', datetime) from tb_comm_status");	// edit 20140513

	data = el1000_sqlite3_select( pSQLite3, query );
	//printf("select size = %d\n", data.size );

	if( data.size > 0 )
	{
		rowCnt = (data.size-9)/9;

		for( i = 0; i < (data.size-9)/9; i++ )
		{
			austemStatusSend[i].nodeid = atoi(data.past_result[rowSize++]);
			austemStatusSend[i].channelSize = atoi(data.past_result[rowSize++]);
			byteOrder = atoi(data.past_result[rowSize++]);
			austemStatusSend[i].channel[0][0]	= (0xff000000 & byteOrder) >> 24;
			austemStatusSend[i].channel[0][1]	= (0x00ff0000 & byteOrder) >> 16;
			austemStatusSend[i].channel[0][2]	= (0x0000ff00 & byteOrder) >> 8;
			austemStatusSend[i].channel[0][3]	= (0x000000ff & byteOrder) >> 0;
			byteOrder = atoi(data.past_result[rowSize++]);
			austemStatusSend[i].channel[1][0]	= (0xff000000 & byteOrder) >> 24;
			austemStatusSend[i].channel[1][1]	= (0x00ff0000 & byteOrder) >> 16;
			austemStatusSend[i].channel[1][2]	= (0x0000ff00 & byteOrder) >> 8;
			austemStatusSend[i].channel[1][3]	= (0x000000ff & byteOrder) >> 0;
			byteOrder = atoi(data.past_result[rowSize++]);
			austemStatusSend[i].channel[2][0]	= (0xff000000 & byteOrder) >> 24;
			austemStatusSend[i].channel[2][1]	= (0x00ff0000 & byteOrder) >> 16;
			austemStatusSend[i].channel[2][2]	= (0x0000ff00 & byteOrder) >> 8;
			austemStatusSend[i].channel[2][3]	= (0x000000ff & byteOrder) >> 0;
			byteOrder = atoi(data.past_result[rowSize++]);
			austemStatusSend[i].channel[3][0]	= (0xff000000 & byteOrder) >> 24;
			austemStatusSend[i].channel[3][1]	= (0x00ff0000 & byteOrder) >> 16;
			austemStatusSend[i].channel[3][2]	= (0x0000ff00 & byteOrder) >> 8;
			austemStatusSend[i].channel[3][3]	= (0x000000ff & byteOrder) >> 0;
			byteOrder = atoi(data.past_result[rowSize++]);
			austemStatusSend[i].channel[4][0]	= (0xff000000 & byteOrder) >> 24;
			austemStatusSend[i].channel[4][1]	= (0x00ff0000 & byteOrder) >> 16;
			austemStatusSend[i].channel[4][2]	= (0x0000ff00 & byteOrder) >> 8;
			austemStatusSend[i].channel[4][3]	= (0x000000ff & byteOrder) >> 0;
			byteOrder = atoi(data.past_result[rowSize++]);
			austemStatusSend[i].channel[5][0]	= (0xff000000 & byteOrder) >> 24;
			austemStatusSend[i].channel[5][1]	= (0x00ff0000 & byteOrder) >> 16;
			austemStatusSend[i].channel[5][2]	= (0x0000ff00 & byteOrder) >> 8;
			austemStatusSend[i].channel[5][3]	= (0x000000ff & byteOrder) >> 0;


			// add 2014.05.13
			//printf("recv time %s\n", data.past_result[rowSize]); 
			yearConvert[0] = data.past_result[rowSize][2];
			yearConvert[1] = data.past_result[rowSize][3];
			austemStatusSend[i].time.year = atoi(yearConvert);

			monthConvert[0] = data.past_result[rowSize][4];
			monthConvert[1] = data.past_result[rowSize][5];
			austemStatusSend[i].time.month = atoi(monthConvert);

			dayConvert[0] = data.past_result[rowSize][6];
			dayConvert[1] = data.past_result[rowSize][7];
			austemStatusSend[i].time.day = atoi(dayConvert);

			hourConvert[0] = data.past_result[rowSize][8];
			hourConvert[1] = data.past_result[rowSize][9];
			austemStatusSend[i].time.hour = atoi(hourConvert);

			minConvert[0] = data.past_result[rowSize][10];
			minConvert[1] = data.past_result[rowSize][11];
			austemStatusSend[i].time.min = atoi(minConvert);

			secConvert[0] = data.past_result[rowSize][12];
			secConvert[1] = data.past_result[rowSize][13];
			austemStatusSend[i].time.sec = atoi(secConvert);

			rowSize++;


			/*
			austemStatusSend[i].channel[1] = atoi(data.past_result[rowSize++]);
			//austemStatusSend[i].channel[1] = htonl(austemStatusSend[i].channel[1]);
			austemStatusSend[i].channel[2] = atoi(data.past_result[rowSize++]);
			//austemStatusSend[i].channel[2] = htonl(austemStatusSend[i].channel[2]);
			austemStatusSend[i].channel[3] = atoi(data.past_result[rowSize++]);
			//austemStatusSend[i].channel[3] = htonl(austemStatusSend[i].channel[3]);
			austemStatusSend[i].channel[4] = atoi(data.past_result[rowSize++]);
			//austemStatusSend[i].channel[4] = htonl(austemStatusSend[i].channel[4]);
			austemStatusSend[i].channel[5] = atoi(data.past_result[rowSize++]);
			//austemStatusSend[i].channel[5] = htonl(austemStatusSend[i].channel[5]);
			*/
		}
		vLength = sizeof(AustemStatusSendData)*rowCnt + 6;	// add 20130820 time 6 byte
		totalLength = vLength + 4;

		tlvSendBuffer[0] = 0xFE;
		tlvSendBuffer[1] = totalLength >> 8;	// total length;
		tlvSendBuffer[2] = totalLength;	// total length;
		tlvSendBuffer[3] = 0x06;		// type;
		tlvSendBuffer[4] = vLength >> 8;	// length;
		tlvSendBuffer[5] = vLength;	// length;
		memcpy(tlvSendBuffer+6, &time, 6);	// add 20130820 time 6 byte
		memcpy(tlvSendBuffer+12, austemStatusSend, vLength);
		tlvSendBuffer[vLength+6] = _getCheckSum( vLength+5 );
		tlvSendBuffer[vLength+7] = 0xFF;

		//int rtrn = write(wifi, tlvSendBuffer, totalLength+4);
		int rtrn = write(client_sock, tlvSendBuffer, totalLength+4);
		//printf("Send rtrn = %d\n", rtrn);

		memset( austemStatusSend, 0, sizeof( AustemStatusSendData)*256); 
		memset( tlvSendBuffer, 0, 20480 ); 

		sqlite3_free( query );
		SQLITE_SAFE_FREE( query )
		sqlite3_free_table( data.past_result );
		SQLITE_SAFE_FREE( data.past_result )

	}
	
	return 0;
}

int getNodeData( QUERYTIME time ) {


	int i;
	int vLength = 0;
	int totalLength = 0;
	char *query;
	char *szErrMsg;
	int	rowSize = 4;
	int	rowCnt = 0;
	char yearConvert[3];
	char monthConvert[3];
	char dayConvert[3];
	char hourConvert[3];
	char minConvert[3];
	char secConvert[3];
	char mSecConvert[4];
	unsigned int byteOrder = 0;
	unsigned short byteOrderSec = 0;

	memset( yearConvert, 0, 3);
	memset( monthConvert, 0, 3);
	memset( dayConvert, 0, 3);
	memset( hourConvert, 0, 3);
	memset( minConvert, 0, 3);
	memset( secConvert, 0, 3);
	memset( mSecConvert, 0, 4);

	Return_Selectdata data;

	/*
	printf("TIME = 20%02d%02d%02d%02d%02d%02d \n",
			time.year,
			time.month,
			time.day,
			time.hour,
			time.minute,
			time.sec
		  );
		  */

	/*
	char testSec;
	if( time.sec == 0 )
	{
		time.minute -= 1;
		testSec = 50;
	}
	else
		testSec = time.sec -10;
		*/
	query = sqlite3_mprintf("select nodeid, tag, value, strftime('%%Y%%m%%d%%H%%M%%f', datetime) from tb_comm_log	\
			where datetime between datetime( strftime('%%s', '20%02d-%02d-%02d %02d:%02d:%02d')-60, 'unixepoch') and '20%02d-%02d-%02d %02d:%02d:%02d.000'",
			time.year,
			time.month,
			time.day,
			time.hour,
			time.minute,
			time.sec,
			time.year,
			time.month,
			time.day,
			time.hour,
			time.minute,
			time.sec
			);

	/*
	query = sqlite3_mprintf("select nodeid, tag, value, strftime('%%Y%%m%%d%%H%%M%%f', datetime) from tb_comm_log	\
			where datetime between '20%02d-%02d-%02d %02d:%02d:%02d.000' and '20%02d-%02d-%02d %02d:%02d:%02d.000'",
			time.year,
			time.month,
			time.day,
			time.hour,
			time.minute,
			//time.sec,
			time.year,
			time.month,
			time.day,
			time.hour,
			time.minute,
			time.sec
			);
			*/

	data = el1000_sqlite3_select( pSQLite3, query );
	//printf("select size = %d\n", data.size );

	if( data.size > 0 )
	{
		//printf("OK data~~\n");
		rowCnt = (data.size-4)/4;

		for( i = 0; i < (data.size-4)/4; i++ )
		{
			austemSend[i].nodeid = atoi(data.past_result[rowSize++]);
			austemSend[i].port	= atoi(data.past_result[rowSize++]);
			byteOrder = 0;
			byteOrder	= atoi(data.past_result[rowSize++]);
			
			austemSend[i].value[0]	= (0xff000000 & byteOrder) >> 24;
			austemSend[i].value[1]	= (0x00ff0000 & byteOrder) >> 16;
			austemSend[i].value[2]	= (0x0000ff00 & byteOrder) >> 8;
			austemSend[i].value[3]	= (0x000000ff & byteOrder) >> 0;

			yearConvert[0] = data.past_result[rowSize][2];
			yearConvert[1] = data.past_result[rowSize][3];
			austemSend[i].time.year = atoi(yearConvert);
			monthConvert[0] = data.past_result[rowSize][4];
			monthConvert[1] = data.past_result[rowSize][5];
			austemSend[i].time.month = atoi(monthConvert);
			dayConvert[0] = data.past_result[rowSize][6];
			dayConvert[1] = data.past_result[rowSize][7];
			austemSend[i].time.day = atoi(dayConvert);
			hourConvert[0] = data.past_result[rowSize][8];
			hourConvert[1] = data.past_result[rowSize][9];
			austemSend[i].time.hour = atoi(hourConvert);
			minConvert[0] = data.past_result[rowSize][10];
			minConvert[1] = data.past_result[rowSize][11];
			austemSend[i].time.min = atoi(minConvert);
			secConvert[0] = data.past_result[rowSize][12];
			secConvert[1] = data.past_result[rowSize][13];
			austemSend[i].time.sec = atoi(secConvert);
			mSecConvert[0] = data.past_result[rowSize][15];
			mSecConvert[1] = data.past_result[rowSize][16];
			mSecConvert[2] = data.past_result[rowSize][17];
			byteOrderSec = 0;
			byteOrderSec = atoi(mSecConvert);
			austemSend[i].time.mSec[0] = (0xff00 & byteOrderSec) >> 8;
			austemSend[i].time.mSec[1] = (0x00ff & byteOrderSec) >> 0;
			//austemSend[i].time.mSec = htons(austemSend[i].time.mSec);
			rowSize++;

		}
		vLength = sizeof(AustemSendData)*rowCnt + 6;	// add 20130820 time 6 byte
		totalLength = vLength + 4;

		tlvSendBuffer[0] = 0xFE;
		tlvSendBuffer[1] = totalLength >> 8;	// total length;
		tlvSendBuffer[2] = totalLength >> 0;	// total length;

		tlvSendBuffer[3] = 0x03;		// type;
		tlvSendBuffer[4] = vLength >> 8;	// length;
		tlvSendBuffer[5] = vLength >> 0;	// length;
		//printf("v Size = %d\n", vLength);
		memcpy(tlvSendBuffer+6, &time, 6);	// add 20130820 time 6 byte
		memcpy(tlvSendBuffer+12, austemSend, vLength);
		tlvSendBuffer[vLength+6] = _getCheckSum( vLength+5 );
		//printf("sum = %d\n", tlvSendBuffer[vLength+6]);
		tlvSendBuffer[vLength+7] = 0xFF;


	}
	else if( data.size == -1 )
	{
		//printf("db lock lock lock \n");
	}
	else
	{
		//printf("no data\n");
		vLength = 6;	// add 20130820 time 6 byte
		totalLength = vLength + 4;

		tlvSendBuffer[0] = 0xFE;
		tlvSendBuffer[1] = totalLength >> 8;	// total length;
		tlvSendBuffer[2] = totalLength;	// total length;
		tlvSendBuffer[3] = 0x03;		// type;
		tlvSendBuffer[4] = vLength >> 8;	// length;
		tlvSendBuffer[5] = vLength;	// length;
		memcpy(tlvSendBuffer+6, &time, 6);	// add 20130820 time 6 byte
		tlvSendBuffer[vLength+6] = _getCheckSum( vLength+5 );
		tlvSendBuffer[vLength+7] = 0xFF;

	}

	//int rtrn = write(wifi, tlvSendBuffer, totalLength+4);
	int rtrn = write(client_sock, tlvSendBuffer, totalLength+4);
	//printf("Send rtrn = %d\n", rtrn);

	//printf("Clear!\n");
	memset( austemSend, 0, sizeof( AustemSendData )*2048 ); 
	memset( tlvSendBuffer, 0, 20480 ); 

	sqlite3_free( query );
	SQLITE_SAFE_FREE( query )
	sqlite3_free_table( data.past_result );
	SQLITE_SAFE_FREE( data.past_result )

		return 0;

}

unsigned char _getCheckSum( int len )
{
	int intSum = 0;
	unsigned char sum;
	int i;

	//printf("start sum %d\n", len);
	for( i = 1; i<= len; i++ )
	{
		intSum += tlvSendBuffer[i];
		//sleep(1);
		//printf("%d ", tlvSendBuffer[i]);
	}

	sum = (unsigned char)intSum;
	//printf("sum %d\n", sum);
	return sum;
}
static int busy(void *handle, int nTry)
{
	printf("[%d] busy handler is called\n", nTry);
	return 0;

}
