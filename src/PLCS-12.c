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
#include "./include/configRW.h"
#include "./include/writeLog.h"
#include "./include/TCPSocket.h"
#include "./include/libpointparser.h"
//#include "./include/UDPConnect.h"

#define SQLITE_SAFE_FREE(x)	if(x){ x = NULL; }
#define	BUFF_SIZE 1024
unsigned char main_quit = 0;

int Socket_Manager( int *fd);
int Pack_Manager( unsigned char* buffer, int len );
static int busy(void *handle, int nTry);
unsigned char _getCheckSum( int len );

int Connect( int port );

typedef struct _PAC {
    char type;
    char length;
    char value[4];

} _PAC;



int m2mid;
int eventid;

unsigned char tlvSendBuffer[20480];

struct sockaddr_in   client_addr;
int client_addr_size;
int tcp;


DEVINFO	    device;
DEVINFO	    saveDev;


sqlite3 *pSQLite3;

int client_sock; 

int plcs_12_id;



int main(int argc, char **argv) { 

    int i;
    pthread_t p_thread;
    int thr_id;
    int status;

    int rc;

    void *socket_fd;
    //int socket_fd;
    struct sockaddr_in servaddr; //server addr

    NODEINFO xmlinfo;
    xmlinfo = pointparser("./tag_info.xml");
    /***************** MSG Queue **********************/
    if( -1 == ( plcs_12_id = msgget( (key_t)1, IPC_CREAT | 0666)))
    {
	    writeLog( ".", "error msgget() plcs_12_id" );
	    //perror( "msgget() ½ÇÆÐ");
	    return -1;
    }

    while(1)
    {
	tcp = TCPServer( 50006 );
	//tcp = TCPClient( "10.1.0.6", "50006");
	printf("Connect %d\n", tcp);
	Socket_Manager( &tcp ); 
    }
    /*******************************************************/
    printf("End\n");

    return 0; 
} 

int Socket_Manager( int *client_sock ) {

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

		FD_SET(*client_sock, &control_msg_readset);
		control_msg_tv.tv_sec = 15;
		//control_msg_tv.tv_usec = 10000;
		control_msg_tv.tv_usec = 5000000;	// timeout check 5 second

		// ¸®ÅÏ°ª -1Àº ¿À·ù¹ß»ý, 0Àº Å¸ÀÓ¾Æ¿ô, 0º¸´Ù Å©¸é º¯°æµÈ ÆÄÀÏ µð½ºÅ©¸³ÅÍ¼ö
		nd = select( *client_sock+1, &control_msg_readset, NULL, NULL, &control_msg_tv );		
		if( nd > 0 ) 
		{

			memset( DataBuf, 0, sizeof(DataBuf) );
			ReadMsgSize = recv( *client_sock, &DataBuf, BUFFER_SIZE, MSG_DONTWAIT);
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
			//shutdown( *client_sock, SHUT_WR );
			break;
		}
		else if( nd == -1 ) 
		{
			printf("error...................\n");
			//shutdown( *client_sock, SHUT_WR );
			break;
		}
		nd = 0;

	}	// end of while

	printf("Disconnection client....\n");

	close( *client_sock );
	return 0;

}

int ParsingReceiveValue(unsigned char* cvalue, int len, unsigned char* remainder, int remainSize )
{

    unsigned char setBuffer[BUFFER_SIZE];
    int i;
    int offset;
    char* token ;
    UINT8 parsing[128][16];
    UINT8 setString[256];
    UINT8 parsingCnt = 1;

    for( i = 0; i < len; i++ )
    {

	if( len == 237 && cvalue[i] == '"' && cvalue[i+1] == ':' && cvalue[i+2] == '"')
	{

		token = strtok( cvalue, ",");
		strcpy( parsing[parsingCnt++], token );
		//printf("[%d] %s\n", parsingCnt-1, parsing[parsingCnt-1] ) ;

		while( token = strtok( NULL, "," ) ) 
		{
		    
		    strcpy( parsing[parsingCnt++], token );
		    //printf("[%d] %s\n", parsingCnt-1, parsing[parsingCnt-1] ) ;
		}

		sprintf(setString, "101:%s 102:%s 103:%s 104:%s 105:%s 106:%s 107:%s 108:%s 111:%s 112:%s 113:%s 114:%s 115:%s 116:%s 117:%s 118:%s"
			, parsing[10], parsing[11], parsing[13], parsing[12], parsing[29]
			, parsing[30], parsing[31], parsing[32], parsing[14], parsing[15]
			, parsing[16], parsing[28], parsing[17], parsing[18], parsing[20]
			, parsing[21]);

		//writeLog( "/work/test", setString);
		printf("%s\n", setString);
	
		selectTag( setString, strlen(setString) );

		return 0;

		/*
	    if( len >= i + cvalue[i+1] && ( cvalue[i + (cvalue[i+1]+2)] == 0xFF ) )
	    {
		memcpy( setBuffer, cvalue+i, cvalue[i+1]+3 );

		i += (cvalue[i+1]+2);

		remainSize = i+1;

		selectTag( setBuffer, i+1 );

	    }
	    */

	}

    }

    remainSize = len - remainSize;
    memcpy( remainder, cvalue+(len-(remainSize)), remainSize );


    return remainSize;
}

int selectTag(unsigned char* buffer, int len )
{

    t_data data;
    int i;
    int offset = 0;
    int cntOffset = 0;
    memset( &data, 0, sizeof(t_data) );

    data.data_type = 1;   // data_type 는 1
    for(i = 0; i < len; i++)
	data.data_buff[offset++] = buffer[i];
    data.data_num = offset; 
 
    if ( -1 == msgsnd( plcs_12_id, &data, sizeof( t_data) - sizeof( long), IPC_NOWAIT))
    {
	//perror( "msgsnd() error ");
	//writeLog( "msgsnd() error : Queue full" );
	sleep(1);
	return -1;
    }

    return 0;
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
	execl("/sbin/hwclock", "/sbin/hwclock", "-w", NULL);
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



    unsigned char sendBuffer[1024];

    int totalLength = 0;
    int vLength = 0;

    type = buffer[2];
    nodeid = buffer[4];

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
	    //rtrn = write(client_sock, tlvSendBuffer, 17);
	    rtrn = sendto( tcp, tlvSendBuffer, 17, 0,( struct sockaddr*)&client_addr, sizeof( client_addr));
	    //printf("Send rtrn = %d\n", rtrn);
	    memset( tlvSendBuffer, 0, 20480 ); 
	    break;


    }

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
