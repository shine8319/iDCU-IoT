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

#include "./include/expat.h" 
#include "./include/sqlite3.h"
#include "./include/PointManager.h"
#include "./include/M2MManager.h"

#define DEBUG
//#define DBPATH "/root/TH1000.sqlite3"
#define DBPATH "/home/root/work/TH1000.sqlite3"


unsigned char user_quit = 0;
unsigned char main_quit = 0;
int	program_start = 1;

extern int el1000_sqlite3_open( char *path, sqlite3 **pSQLite3 );
extern int el1000_sqlite3_customize( sqlite3 **pSQLite3 );
extern int el1000_sqlite3_close( sqlite3 **pSQLite3 );
extern int el1000_sqlite3_transaction( sqlite3 **pSQLite3, char status );
extern Return_Selectdata el1000_sqlite3_select( sqlite3 *pSQLite3, char *query );
extern NODEINFO pointparser(const char *path);
extern DEVINFO deviceparser(const char *path);

//void* Connect_Manager( char *ip, char *port );
int _Connect_Manager(sqlite3 *pSQLite3, unsigned short port);
int _Socket_Manager( sqlite3 *pSQLite3 );
//int Socket_Manager( void* socket_fd, sqlite3 *pSQLite3 );
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


int server_sock;
int client_sock;


DEVINFO dev ;

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

	int rc;
	sqlite3 *pSQLite3;

	void *socket_fd;
	struct sockaddr_in servaddr; //server addr

	if (argc < 3) {
		printf ("%d ",argc);
		fprintf(stderr, "Usage: %s <Port> <DB Path>\n", argv[0]);
		//exit(2);
		return -1;
	}

	(void)signal(SIGINT, quitsignal);
	printf("Press <ctrl+c> to user_quit. \n");

	/******************** DB connect ***********************/
	//char	*szErrMsg;

	// DB Open
	//rc = el1000_sqlite3_open( DBPATH, &pSQLite3 );
	rc = el1000_sqlite3_open( argv[2], &pSQLite3 );
	if( rc != 0 )
	{
		return -1;
	}
	else
		printf("%s OPEN!!\n", argv[2]);

	// DB Customize
	rc = el1000_sqlite3_customize( &pSQLite3 );
	if( rc != 0 )
		return -1;



	dev = deviceparser("RTD_info.xml");
	/*******************************************************/


	/***************** Server Connect **********************/
	

	//while( !main_quit) 
	{

	_Connect_Manager(pSQLite3, atoi(argv[1]) );

		/*
		socket_fd = Connect_Manager( argv[1], argv[1] );
		printf(" socket fd = %d\n", socket_fd );
		if( -1 == socket_fd )
		{
			//close( socket_fd );
			//sleep(5);
			continue;
		}
		user_quit = 0;


		Socket_Manager( socket_fd, pSQLite3 );

		printf("TCP : Client No Signal. Connection Try Again....\n");
		*/
		//close( socket_fd );

	}


	/*******************************************************/
	el1000_sqlite3_close( &pSQLite3 );

	printf("[M2MManager] end of main loop \n");
	return 0; 
} 


int _Connect_Manager(sqlite3 *pSQLite3, unsigned short port) {

	struct sockaddr_in servaddr; //server addr
	struct sockaddr_in clientaddr; //client addr
	//unsigned short port = 3030;
	int client_addr_size;
	int bufsize = 100000;
	int rn = sizeof(int);
	int flags;
	pid_t pid;


	/*
	act.sa_handler = z_handler;
	sigemptyset(&act.sa_mask);
	act.sa_flags = 0;

	state = sigaction(SIGCHLD, &act, 0);
	if( state != 0 )
	{
		puts("Sigaction() error");
		exit(1);
	}
	*/
	printf("Start Socket Server!!\n");


	while( !main_quit) 
	{

		user_quit = 0;
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
		//pthread_create(&thread, NULL, Socket_Manager, (void*)client_sock);
		//Socket_Manager( &server_sock );
		//Socket_Manager();

		_Socket_Manager(  pSQLite3 );
		printf("TCP : Client No Signal. Connection Try Again....\n");

	}

	close( client_sock );

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
	struct sockaddr_in clientaddr; //client addr
	//unsigned short port = 2020;
	int client_addr_size;
	int bufsize = 100000;
	int rn = sizeof(int);
	int flags;
	pid_t pid;
	int server_sock;
	int client_sock;

	printf("Start Socket Server!!\n");



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
		printf("bind\n");

		listen(server_sock, 5);
		printf("listen\n");

		client_addr_size = sizeof(clientaddr);
		client_sock = accept(server_sock, (struct sockaddr*)&clientaddr, &client_addr_size);
		printf("accept\n");


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
		//pthread_create(&thread, NULL, Socket_Manager, (void*)client_sock);
		//Socket_Manager( &server_sock );
		//Socket_Manager();

	//close( client_sock );
	return (void*)client_sock;


}

#define SQLITE_SAFE_FREE(x)	if(x){ x = NULL; }
/*
int Socket_Manager( void* socket_fd, sqlite3 *pSQLite3 ) {

	int sock = (int)socket_fd;
	fd_set control_msg_readset;
	struct timeval control_msg_tv;

	int rtrn;
	int i;
	int nd;
	int sendresult = 0;

	int insert_start = 0;
	int loop = 1;
	int merge_ok = 0;
	int second_command = 0; 
	int second_command_flag = 0; 


	while( loop ) 
	{
		loop = 0;

	}	// end of while

	printf("Disconnection client....\n");

	close( client_sock );
	return 0;

}

*/

int _Socket_Manager( sqlite3 *pSQLite3 ) {
//int Socket_Manager( void* socket_fd, sqlite3 *pSQLite3 ) {

	//int sock = (int)socket_fd;
	int sock = client_sock;
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

	char *query;
	char **past_result;
	Return_Selectdata data;

	int tagcnt;

	int error = 0;  
	int esize;  
	int n;


	if( program_start )
	{
		program_start = 0;
		tag = pointparser("usn_node_info.xml");

		printf("tag.getPointSize = %d\n", tag.getPointSize);
	}


	while( !user_quit ) 
	{


		for( tagcnt = 0; tagcnt < tag.getPointSize; tagcnt++)
		{
			query = sqlite3_mprintf("select nodeid, temperature, humidity, lux, bat from tb_th1000_data where nodeid = '%d' and alive = '1' order by id desc limit 1",
			//query = sqlite3_mprintf("select nodeid, temperature, humidity, lux, bat from tb_th1000_data where nodeid = '%d' order by id desc limit 1",
					tag.id[tagcnt]		
					);
			
			data = el1000_sqlite3_select( pSQLite3, query );
			printf("nodeid = %d, select size = %d\n", tag.id[tagcnt], data.size );

			if( data.size > 0 )
			{
				for( i = 5; i < 10; i++ )
				{
					if( data.past_result[i] == NULL )
					{
						printf("data Error (NULL)\n");
						data.size = 0;
					}
				}
			}

			atoi_cnt = 0;
			if( data.size > 0 )
			{

				for( i = 5; i < 10; i++ )
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

						//send_buff[buff_cnt++] = 0x13;
						send_buff[buff_cnt++] = 0x0C;
						send_buff[buff_cnt++] = (char)(parsingvalue >> 8);
					}

					send_buff[0] = buff_cnt-1;
					send_buff[0] <<= 1;
					send_buff[0] |= 0x1;


					printf("%d | ", selectdata[i]);
				} //  for( i = 0; i < atoi_cnt; i++ )
				printf("\n");

				esize = sizeof(int);  
				if ((n = getsockopt(sock, SOL_SOCKET, SO_ERROR, &error, (socklen_t *)&esize)) < 0)  
				{
					printf("error n = %d\n", n);
					//return -1;  
				}
				else
					printf("ok n = %d\n", n);

				for( i = 0; i < buff_cnt; i++ )
				{
					printf("%02X ", send_buff[i]);
				}
				printf("\n\n");

				//printf("sock = %d\n", sock);
				//printf("pre errno = %d\n", errno );
				send_size = send( sock, &send_buff, buff_cnt, MSG_NOSIGNAL );
				//printf("errno = %d\n", errno );
				//printf("send data size : %d\n", send_size );
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


		//sleep(10);

		esize = sizeof(int);  
		if ((n = getsockopt(sock, SOL_SOCKET, SO_ERROR, &error, (socklen_t *)&esize)) < 0)  
		{
			printf("error n = %d\n", n);
			user_quit = 1;
			send_size = -1;
			//return -1;  
		}
		else
			printf("ok n = %d\n", n);


		if( send_size != -1 )
		{
			printf("(int)dev.expiretime[0] = %d\n", (int)dev.expiretime[0]);
			sleep( (int)dev.expiretime[0] );
		}

	}	// end while();

	printf("socket fd close...\n");
	close( sock );

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




