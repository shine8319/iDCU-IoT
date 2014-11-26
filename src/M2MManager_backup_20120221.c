#include <stdio.h> 
#include <stdlib.h> 
#include <string.h> 
#include <unistd.h>
#include <sys/ipc.h>
#include <sys/shm.h>
#include <signal.h>
#include <sys/socket.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <arpa/inet.h>
#include <fcntl.h>
#include <errno.h>
#include <time.h>

#include "/usr/local/arm/expat/arm/include/expat.h" 
#include "/work/RnD/weather/include/sqlite3.h"
#include "/work/RnD/weather/include/PointManager.h"
#include "/work/RnD/weather/include/M2MManager.h"

#define DEBUG
#define DBPATH "/root/TH1000.sqlite3"


unsigned char user_quit = 0;
unsigned char main_quit = 0;
int	program_start = 1;

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
//int ConnectWait(int sockfd, struct sockaddr *saddr, int addrsize, int sec);
int ConnectWait(void* sockfd, struct sockaddr *saddr, int addrsize, int sec);
static void sig_handler(int signo);
void *t_function(void *data);

typedef struct _PAC {
	char type;
	char length;
	char value[4];

} _PAC;

void quitsignal(int sig) 
{
	user_quit = 1;
	main_quit = 1;
}

int main(int argc, char **argv) { 

	int i;
	pthread_t p_thread;
	int thr_id;
	int status;

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
	
		void *socket_fd;
		struct sockaddr_in servaddr; //server addr

	while( !main_quit) 
	{

		socket_fd = Connect_Manager( argv[1], argv[2] );
		printf(" socket fd = %d\n", socket_fd );
		if( -1 == socket_fd )
		{
			//close( socket_fd );
			//sleep(5);
			continue;
		}
		user_quit = 0;


		Socket_Manager( socket_fd, pSQLite3 );
		//close( socket_fd );

	}


	/*******************************************************/
	el1000_sqlite3_close( &pSQLite3 );

	printf("[M2MManager] end of main loop \n");
	return 0; 
} 
void *t_function(void *fd) 
{ 
    int sock; 
	char send_buff[1024] = "12345";
	int send_size = 0;

    sock = *((int *)fd); 
 
	while( !user_quit )
	{

		sleep(1);
		send_size = send( sock, &send_buff, 5, 0 );
		printf(" send_size = %d\n", send_size );
		printf("errno = %d\n", errno );
		if( send_size < 0 )
		{
			user_quit = 1;
			close( sock );
		}
	}

} 

int ConnectWait(void *arg_sockfd, struct sockaddr *saddr, int addrsize, int sec)  
{  
	int sockfd = (int)arg_sockfd;
	int newSockStat;  
	int orgSockStat;  
	int res, n;  
	fd_set  rset, wset;  
	struct timeval tval;  

	int error = 0;  
	int esize;  

	if ( (newSockStat = fcntl(sockfd, F_GETFL, NULL)) < 0 )   
	{  
		perror("F_GETFL error");  
		return -1;  
	}  

	orgSockStat = newSockStat;  
	newSockStat |= O_NONBLOCK;  
	if( newSockStat & O_NONBLOCK )
		printf("NOBLOCK Setting\n");

	// Non blocking 상태로 만든다.   
	if(fcntl(sockfd, F_SETFL, newSockStat) < 0)  
	{  
		perror("F_SETLF error");  
		return -1;  
	}  

	printf("pre errno = %d\n", errno );
	// 연결을 기다린다.  
	// Non blocking 상태이므로 바로 리턴한다.  
	if((res = connect(sockfd, saddr, addrsize)) < 0)  
	{  
		printf("errno = %d\n", errno );
		if (errno != EINPROGRESS)  
			return -1;  
	}  

	printf("RES : %d\n", res);  


	// 즉시 연결이 성공했을 경우 소켓을 원래 상태로 되돌리고 리턴한다.   
	if (res == 0)  
	{  
		printf("Connect Success\n");  
		fcntl(sockfd, F_SETFL, orgSockStat);  
		return 1;  
	}  

	FD_ZERO(&rset);  
	FD_SET(sockfd, &rset);  
	wset = rset;  

	tval.tv_sec     = sec;      
	tval.tv_usec    = 0;  

	if ( (n = select(sockfd+1, &rset, &wset, NULL, NULL)) == 0)  
	{  
		// timeout  
		errno = ETIMEDOUT;      
		return -1;  
	}  

	// 읽거나 쓴 데이터가 있는지 검사한다.   
	if (FD_ISSET(sockfd, &rset) || FD_ISSET(sockfd, &wset) )  
	{  
		printf("Read data\n");  
		esize = sizeof(int);  
		if ((n = getsockopt(sockfd, SOL_SOCKET, SO_ERROR, &error, (socklen_t *)&esize)) < 0)  
			return -1;  
	}  
	else  
	{  
		perror("Socket Not Set");  
		return -1;  
	}  


	fcntl(sockfd, F_SETFL, orgSockStat);  
	if(error)  
	{  
		errno = error;  
		perror("Socket");  
		return -1;  
	}  

	return 1;  
}  

static void sig_handler(int signo) 
{ 
	printf("Alarm: Server Connect TimeOUT...\n");
    return; 
} 

void* Connect_Manager( char *ip, char *port ) 
{
	struct sockaddr_in servaddr; //server addr
	//int bufsize = 100000;
	//int rn = sizeof(int);
	int server_sock;

	// signal 설정 
    struct sigaction sigact, oldact; 
    sigact.sa_handler = sig_handler; 
    sigemptyset(&sigact.sa_mask); 
    sigact.sa_flags = 0; 
    // 바로 이부분이 SIGNAL 을 받았을때  
    // Interrupt 가 발생하도록 설정하는 부분이다.   
    sigact.sa_flags |= SA_INTERRUPT; 
 
    if (sigaction(SIGALRM, &sigact, &oldact) < 0) 
    { 
        perror("sigaction error : "); 
        exit(0); 
    } 

	printf("Start Socket !!\n");


	//	while( !user_quit ) {

	/*
	SOCK_DGRAM	= 1,
	SOCK_STREAM	= 2,
	SOCK_RAW	= 3,
	SOCK_RDM	= 4,
	SOCK_SEQPACKET	= 5,
	SOCK_DCCP	= 6,
	SOCK_PACKET	= 10,
	*/

	/* create socket */
	server_sock = socket(PF_INET, SOCK_STREAM, 0);
	//server_sock = socket(AF_INET, SOCK_PACKET, 0);
	if( server_sock < 0 )
	{
		perror("error : ");
		exit(0);
	}

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

	/*
	if (ConnectWait( (void *)server_sock, (struct sockaddr *)&servaddr, sizeof(servaddr), 5) < 0)  
	{  
		printf("ConnectWait Error return \n");
		perror("error");  
		close( server_sock );
		return -1;
	}  
	else  
	{  
		printf("Connect Success\n");  
	}  
	*/
	alarm(5);

	int rtrn;
	rtrn = connect(server_sock, (struct sockaddr *)&servaddr, sizeof(servaddr));
	printf( "rtrn = %d\n", rtrn );
	printf( "errno = %d\n", errno );
	if( rtrn == -1 )
	{
		close( server_sock );
		return -1;
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
	NODEINFO tag;
	if( program_start )
	{
		program_start = 0;
		tag = pointparser("usn_node_info.xml");

		printf("tag.getPointSize = %d\n", tag.getPointSize);
	}


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
			printf("select size = %d\n", data.size );

			atoi_cnt = 0;
			if( data.size > 0 )
			{

				for( i = 7; i < 12; i++ )
				{
					selectdata[atoi_cnt++] =  atoi(data.past_result[i]);
				}


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
					memcpy( pac[i].value, &parsingvalue, parsinglength );
					if( parsinglength == 1 )
					{
						send_buff[buff_cnt++] = (char)selectdata[i];
					}
					else if( parsinglength == 2 )
					{
						send_buff[buff_cnt++] = pac[i].value[_1byte];
						send_buff[buff_cnt++] = pac[i].value[_2byte];
					}
					else if( parsinglength == 3 )
					{
						send_buff[buff_cnt++] = pac[i].value[_1byte];
						send_buff[buff_cnt++] = pac[i].value[_2byte];
						send_buff[buff_cnt++] = pac[i].value[_3byte];
					}
					else if( parsinglength == 4 )
					{
						send_buff[buff_cnt++] = pac[i].value[_1byte];
						send_buff[buff_cnt++] = pac[i].value[_2byte];
						send_buff[buff_cnt++] = pac[i].value[_3byte];
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
					}

					send_buff[0] = buff_cnt-1;
					send_buff[0] <<= 1;
					send_buff[0] |= 0x1;


					printf("%d | ", selectdata[i]);
				} //  for( i = 0; i < atoi_cnt; i++ )
				printf("\n");

				int error = 0;  
				int esize;  
				int n;

				esize = sizeof(int);  
				if ((n = getsockopt(sock, SOL_SOCKET, SO_ERROR, &error, (socklen_t *)&esize)) < 0)  
				{
					printf("error n = %d\n", n);
					return -1;  
				}
				else
					printf("ok n = %d\n", n);

				printf("sock = %d\n", sock);
				printf("pre errno = %d\n", errno );
				send_size = send( sock, &send_buff, buff_cnt, 0 );
				printf("errno = %d\n", errno );
				printf("send data size : %d\n", send_size );
				if( send_size == -1 )
				{
					printf("Send Error.... ");
					user_quit = 1;
				}
				buff_cnt = 1;


			}  // if( data.size > 0 )

			sqlite3_free( query );
			SQLITE_SAFE_FREE( query )
			sqlite3_free_table( data.past_result );
			SQLITE_SAFE_FREE( data.past_result )
		
		} // for( tagcnt = 0; tagcnt < tag.getPointSize; tagcnt++)


		sleep(10);

	}	// end while();

	printf("socket fd close...\n");
	//close( sock );

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




