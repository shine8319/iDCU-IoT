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
#include "./include/writeLog.h"
#include "./include/getDiffTime.h"
#include "./include/sqlite3.h"
#include "./include/sqliteManager.h"

#define DBPATH "/work/db/comm"
#define SQLITE_SAFE_FREE(x)	if(x){ x = NULL; }
#define BUFFER_SIZE 2048
#define	BAUDRATE	115200	
unsigned char _getCheckSum( int len );
unsigned char _uartGetCheckSum( unsigned char* buffer, int len );
void *thread_syncCount(void *arg);
void *thread_main(void *);
void *ConnectPorcess();
void sig_handler( int signo);
int setCountClear(UINT8 port );


pthread_t threads[5];
int m2mid;
int eventid;

StatusSendData statusSend[8];
UINT8 tlvSendBuffer[BUFFER_SIZE];

MGData pastData;
MGData node;
UINT8 io[8];


sqlite3 *pSQLite3;

int server_sock; 
int client_sock; 
int fd; // Serial Port File Descriptor

int main(int argc, char **argv) { 

    int i;
    pthread_t p_thread;
    int thr_id;
    int status;

    int rc;

    void *socket_fd;
    struct sockaddr_in servaddr; //server addr

    // wifi
    struct termios tio, old_tio;
    int ret;

    unsigned char DataBuf[BUFFER_SIZE];
    int ReadMsgSize;

    unsigned char receiveBuffer[BUFFER_SIZE];
    int receiveSize = 0;
    unsigned char remainder[BUFFER_SIZE];
    int parsingSize = 0;

    char th_data[256];

    memset( &pastData, 0, sizeof( MGData ) );
    memset( &io, 0, sizeof( UINT8 )*8 );

    signal( SIGINT, (void *)sig_handler);
    fd = OpenSerial();
    if( fd == -1 )
    {
	writeLog("/work/iot/log", "Uart Open Fail." );
	return -1;
    }

    /******************** DB connect ***********************/
    // DB Open
    rc = _sqlite3_open( DBPATH, &pSQLite3 );
    if( rc != 0 )
    {
	writeLog( "/work/iot/log", "error DB Open" );
	return -1;
    }
    else
    {
	writeLog( "/work/iot/log", "DB Open" );
	printf("%s OPEN!!\n", DBPATH);
    }


    //sqlite3_busy_timeout( pSQLite3, 1000);
    //sqlite3_busy_timeout( pSQLite3, 5000);

    // DB Customize
    rc = _sqlite3_customize( &pSQLite3 );
    if( rc != 0 )
    {
	writeLog( "/work/iot/log", "error DB Customize" );
	return -1;
    }
    rc = _sqlite3_nolock( &pSQLite3 );
    if( rc != 0 )
    {
	writeLog( "/work/iot/log", "error set nolock" );
	return -1;
    }

    rc = init();
    if( rc == -1 )
    {
	writeLog( "/work/iot/log", "error init()" );
	return -1;
    }

    /***************** Server Connect **********************/
    if( -1 == ( m2mid = msgget( (key_t)2222, IPC_CREAT | 0666)))
    {
	writeLog( "/work/iot/log", "error msgget() m2mid" );
	return -1;
    }

    if( -1 == ( eventid = msgget( (key_t)3333, IPC_CREAT | 0666)))
    {
	writeLog( "/work/iot/log", "error msgget() eventid" );
	return -1;
    }

    memcpy( th_data, (void *)&fd, sizeof(fd) );
    if( pthread_create(&threads[2], NULL, &thread_main, (void *)th_data) == -1 )
    {
	writeLog("/work/iot/log", "[Uart] threads[2] thread_main error" );
	printf("error thread\n");
    }

    if( pthread_create(&threads[1], NULL, &ConnectPorcess, NULL ) == -1 )
    {
	writeLog("/work/iot/log", "[TCP/IP] thread Create error" );
    }
    usleep(1000);

    while(1)
    { 

	// receive Bridge
	ReadMsgSize = read(fd, DataBuf, BUFFER_SIZE); 
	if(ReadMsgSize > 0)
	{

	    if( receiveSize >= BUFFER_SIZE )
		continue;


	    /*
	       printf("STX ");
	       for( i = 0; i< ReadMsgSize; i++ )
	       printf("%02X ", DataBuf[i]);
	       printf(" ETX\n");
	       */

	    memcpy( receiveBuffer+receiveSize, DataBuf, ReadMsgSize );
	    receiveSize += ReadMsgSize;

	    //printf("receiveSize = %d\n", receiveSize );

	    parsingSize = uartParsingReceiveValue(receiveBuffer, receiveSize, remainder, parsingSize);

	    //printf("parsingSize(%d) ", parsingSize);

	    memset( receiveBuffer, 0 , sizeof(BUFFER_SIZE) );
	    receiveSize = 0;
	    memcpy( receiveBuffer, remainder, parsingSize );
	    receiveSize = parsingSize;

	    parsingSize = 0;	// add 20140508
	    memset( remainder, 0 , sizeof(BUFFER_SIZE) );
	}
	else
	{
	    //usleep(100);
	    //printf("Serial timeout %d\n", ReadMsgSize);
	}
    }




    /*******************************************************/
    _sqlite3_close( &pSQLite3 );

    writeLog( "/work/iot/log", "[IoTManager] END" );
    return 0; 
} 

void sig_handler( int signo)
{
    int i;
    int rc;
    int status;

    printf("ctrl-c\n");

    for (i = 4; i >= 0; i--)
    {
	rc = pthread_cancel(threads[i]); // 강제종료
	if (rc == 0)
	{
	    // 자동종료
	    rc = pthread_join(threads[i], (void **)&status);
	    if (rc == 0)
	    {
		printf("Completed join with thread %d status= %d\n",i, status);
	    }
	    else
	    {
		printf("ERROR; return code from pthread_join() is %d, thread %d\n", rc, i);
	    }
	}
    }
    exit(1);
}

int init( void ) {

    char *query;
    int i,j;
    int	rowSize = 8;
    unsigned int byteOrder = 0;

    SQLite3Data data;

    query = sqlite3_mprintf("select port1, port2, port3, port4, port5, port6, port7, port8 from tb_comm_status");	// edit 20140513


    data = _sqlite3_select( pSQLite3, query );


    if( data.size > 0 )
    {

	for( i = 0; i < (data.size-8)/8; i++ )
	{

	    for( j = 0; j < 8; j++ )
	    {
		pastData.cnt[j] = atoi(data.data[rowSize++]);
		node.cnt[j] = pastData.cnt[j];

	    }

	}
    }
    else
	return -1;


    return 0;
}

int SocketPorcess( void ) {

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

	// ¸®ÅÏ°ª -1Àº ¿À·ù¹ß»ý, 0Àº Å¸ÀÓ¾Æ¿ô, 0º¸´Ù Å©¸é º¯°æµÈ ÆÄÀÏ µð½ºÅ©¸³ÅÍ¼ö
	nd = select( client_sock+1, &control_msg_readset, NULL, NULL, &control_msg_tv );		
	if( nd > 0 ) 
	{

	    memset( DataBuf, 0, sizeof(DataBuf) );
	    ReadMsgSize = recv( client_sock, &DataBuf, BUFFER_SIZE, MSG_DONTWAIT);
	    if( ReadMsgSize > 0 ) 
	    {
		/*
		   for( i = 0; i < ReadMsgSize; i++ ) {
		   printf("%-3.2x", DataBuf[i]);
		   }
		   printf("\n");
		   printf("recv data size : %d\n", ReadMsgSize);
		   */

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


void *ConnectPorcess() {

    struct sockaddr_in servaddr; //server addr
    struct sockaddr_in clientaddr; //client addr
    unsigned short port = 502;
    //unsigned short port = 9000;
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
	    return;
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
	SocketPorcess();
	printf("TCP : Client No Signal. Connection Try Again....\n");

    }

    close( client_sock );

}

int ParsingReceiveValue(unsigned char* cvalue, int len, unsigned char* remainder, int remainSize )
{

    unsigned char setBuffer[BUFFER_SIZE];
    int i;
    int size;
    int offset;

    for( i = 0; i < len; i++ )
    {

	if( cvalue[i] == 0x01 && cvalue[i+1] == 0x02 )
	{

	    size = cvalue[i+5]+6;

	    memcpy( setBuffer, cvalue+i, size );

	    /*
	       for( offset = 0; offset < size; offset++ )
	       printf("%02X ", setBuffer[offset]);
	       printf("\n");
	     */


	    i += size-1;

	    remainSize = i+1;

	    CommandProcess( setBuffer, size );


	}

    }

    remainSize = len - remainSize;
    memcpy( remainder, cvalue+(len-(remainSize)), remainSize );


    return remainSize;
}

int AustemTimeSync( TimeOrigin time )
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
	    time.min,
	    time.year,
	    time.sec );

    if( time.year < 12 || time.year > 99 
	    || time.month < 1 || time.month > 12 
	    || time.day < 1 || time.day > 31 
	    || time.hour > 24 
	    || time.min> 59
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

int CreateSync( UINT8 type ) 
{
    char th_data[256];
    memcpy( th_data, (void *)&fd, sizeof(fd) );

    switch( type )
    {
	case 1:
	    if( pthread_create(&threads[0], NULL, &thread_syncCount, (void *)th_data) == -1 )
	    {
		writeLog("/work/iot/log", "[Uart] threads[0] syncCount error" );
	    }
	    break;

	case 0:

	    break;

    }

    return 0;
}
int receiveEvent( unsigned char* buffer, int len ) 
{
    int rc;
    int status;
    int cntOffset;
    int transaction = 0;
    UINT8 i;
    UINT8 changed = 0;

    cntOffset = 4;

    switch( buffer[2] )
    {
	case 1:
	    CreateSync( buffer[4] );
	    break;
	default:
	    for( i = 0; i < 8; i++ )
	    {
		node.cnt[i] = 0;

		node.cnt[i] = (0x000000ff & (UINT32)buffer[cntOffset++]) << 24;
		node.cnt[i] |= (0x000000ff & (UINT32)buffer[cntOffset++]) << 16;
		node.cnt[i] |= (0x000000ff & (UINT32)buffer[cntOffset++]) << 8;
		node.cnt[i] |= (0x000000ff & (UINT32)buffer[cntOffset++]) << 0;

		cntOffset += 2;

		if( pastData.cnt[i] != node.cnt[i] )
		    changed = 1;

	    }
	    for( i = 0; i < 8; i++ )
	    {
		io[i] = 0;
		io[i] = buffer[cntOffset++];

		cntOffset += 2;
	    }

	    if( changed )
	    {
		transaction = _sqlite3_transaction( &pSQLite3, 1 ) ;
		if( transaction == 0 )
		{
		    _sqlite3_update( &pSQLite3, node );
		    do {
			transaction = _sqlite3_transaction( &pSQLite3, 2 ) ;
		    } while( transaction );

		}

		for( i = 0; i < 8; i++ )
		    printf("[%d] %ld/%ld ", i, pastData.cnt[i], node.cnt[i] );
		printf("\n");
	    }

	    for( i = 0; i < 8; i++ )
	    {
		//value[id].curWo[i] = node.wo[i];
		pastData.cnt[i] = node.cnt[i];
	    }

	    break;
    }

}

int CommandProcess( unsigned char* buffer, int len ) 
{
    int i;
    int rtrn;
    UINT8 type;
    UINT8 port;

    type = buffer[7];

    switch( type )
    {
	case 2:
	    getIOStatus( );
	    break;
	case 4:
	    getCommStatus( );
	    break;
	case 5:
	    port = buffer[9];
	    setCountClear( port );
	    break;
    }

    return 0;

}


int setCountClear(UINT8 port ) {

    int i;
    int rtrn;
    int length;
    unsigned char SendBuf[BUFFER_SIZE];
    memset(SendBuf, 0, BUFFER_SIZE);


    SendBuf[0] = 0xFE;
    SendBuf[1] = 0x04;
    SendBuf[2] = 0x55;
    SendBuf[3] = 0x02;
    SendBuf[4] = 0x02;	// port clear
    SendBuf[5] = port;
    SendBuf[6] = 0xFF;

    rtrn = write(fd, SendBuf, 7); 
    printf(" send %d~~~~~~~~~~~~~~\n", rtrn);



    return 0;
}

int getIOStatus( ) {

    UINT8   SendBuffer[32];
    UINT8   i;

    SendBuffer[0] = 0x01;
    SendBuffer[1] = 0x02;
    SendBuffer[2] = 0x00;
    SendBuffer[3] = 0x00;
    SendBuffer[4] = 0x00;
    SendBuffer[5] = 0x04;    // t length
    SendBuffer[6] = 0x01;
    SendBuffer[7] = 0x02;
    SendBuffer[8] = 0x01;

    SendBuffer[9] = 0;
    SendBuffer[9] = io[0] << 0;
    SendBuffer[9] |= io[1] << 1;
    SendBuffer[9] |= io[2] << 2;
    SendBuffer[9] |= io[3] << 3;
    SendBuffer[9] |= io[4] << 4;
    SendBuffer[9] |= io[5] << 5;
    SendBuffer[9] |= io[6] << 6;
    SendBuffer[9] |= io[7] << 7;


    int rtrn = write(client_sock, SendBuffer, SendBuffer[5]+6 );
    /*
    for( i = 0; i < rtrn; i++ ) {
	printf("%-3.2x", SendBuffer[i]);
    }
    printf("\n");
    */

    return 0;
}


int getCommStatus( ) {

    int i,j;
    char *query;
    char *szErrMsg;
    int	rowSize = 8;
    int	rowCnt = 0;
    unsigned int byteOrder = 0;
    int	transaction = 0;

    SQLite3Data data;

    query = sqlite3_mprintf("select port1, port2, port3, port4, port5, port6, port7, port8 from tb_comm_status");	// edit 20140513


    data = _sqlite3_select( pSQLite3, query );


    if( data.size > 0 )
    {
	rowCnt = (data.size-8)/8;

	for( i = 0; i < (data.size-8)/8; i++ )
	{

	    for( j = 0; j < 8; j++ )
	    {
		byteOrder = atoi(data.data[rowSize++]);

		statusSend[i].channel[j][0]	= (0x0000ff00 & byteOrder) >> 8;
		statusSend[i].channel[j][1]	= (0x000000ff & byteOrder) >> 0;
		statusSend[i].channel[j][2]	= (0xff000000 & byteOrder) >> 24;
		statusSend[i].channel[j][3]	= (0x00ff0000 & byteOrder) >> 16;
	    }

	}

	tlvSendBuffer[0] = 0x01;
	tlvSendBuffer[1] = 0x02;
	tlvSendBuffer[2] = 0x00;
	tlvSendBuffer[3] = 0x00;
	tlvSendBuffer[4] = 0x00;
	tlvSendBuffer[5] = 0x23;    // t length
	tlvSendBuffer[6] = 0x01;
	tlvSendBuffer[7] = 0x04;
	tlvSendBuffer[8] = 0x20;

	memcpy(tlvSendBuffer+9, statusSend, sizeof( StatusSendData));

	int rtrn = write(client_sock, tlvSendBuffer, sizeof( StatusSendData)+9);
	/*
	for( i = 0; i < rtrn; i++ ) {
	    printf("%-3.2x", tlvSendBuffer[i]);
	}
	printf("\n");
	*/

	memset( statusSend, 0, sizeof( StatusSendData)*8); 
	memset( tlvSendBuffer, 0, 2048 ); 

	sqlite3_free( query );
	SQLITE_SAFE_FREE( query )
	    sqlite3_free_table( data.data );
	SQLITE_SAFE_FREE( data.data )

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

int OpenSerial(void)
{
    int fd; // File descriptor
    struct termios newtio;

    int rtrn;

    fd = open( "/dev/ttyAMA0", O_RDWR | O_NOCTTY);


    if(fd < 0)
    {
	printf("Bridge Open Fail.\n");
	return -1;
    }
    memset(&newtio, 0, sizeof(newtio));
    newtio.c_iflag = IGNPAR; // Parity error 문자 바이트 무시
    newtio.c_oflag = 0; // 2
    newtio.c_cflag = CS8|CLOCAL|CREAD; // 8N1, Local Connection, 문자(char형)수신 가능

    switch(BAUDRATE) // Baud Rate
    {
	case 115200 : newtio.c_cflag |= B115200;
		      break;
	case 57600 : newtio.c_cflag |= B57600;
		     break;
	case 38400 : newtio.c_cflag |= B38400;
		     break;
	case 19200 : newtio.c_cflag |= B19200;
		     break;
	case 9600: newtio.c_cflag |= B9600;
		   break;
	default : newtio.c_cflag |= B57600;
		  break;
    }
    newtio.c_lflag = 0;	    // Non-Canonical 입력 처리
    newtio.c_cc[VTIME] = 1;// 0.1초 이상 수신이 없으면 timeout. read()함수는 0을 리턴
    newtio.c_cc[VMIN] = 0;

    tcflush(fd, TCIFLUSH); // Serial Buffer initialized
    tcsetattr(fd, TCSANOW, &newtio); // Modem line initialized & Port setting 종료

    return fd;
}

int CloseSerial(int fd)
{
    close(fd);
}
void *thread_syncCount(void *arg)
{

    int i;
    int rtrn;
    int uartFd;
    int length;
    unsigned char SendBuf[BUFFER_SIZE];
    memset(SendBuf, 0, BUFFER_SIZE);

    memcpy( (void *)&uartFd, (char *)arg, sizeof(int));

    length = sizeof( MGData );

    SendBuf[0] = 0xFE;
    SendBuf[1] = length + 3;
    SendBuf[2] = 0x32;
    SendBuf[3] = length;

    memcpy( SendBuf+4, &pastData, sizeof( MGData ) );
    SendBuf[length+3] = 0xFF;

    for( i = 0; i < length+4; i++ )
    {
	printf("%02X ", SendBuf[i]);
    }
    printf("\n");
    //usleep(200);
    rtrn = write(uartFd, SendBuf, length+4); 
    printf(" send %d~~~~~~~~~~~~~~\n", rtrn);


    pthread_exit( (void *) 0);
}

void *thread_main(void *arg)
{

    int rtrn;
    int uartFd;
    int length;
    unsigned char SendBuf[BUFFER_SIZE];
    memset(SendBuf, 0, BUFFER_SIZE);

    memcpy( (void *)&uartFd, (char *)arg, sizeof(int));

    SendBuf[0] = 0xFE;
    SendBuf[1] = 0x03;
    SendBuf[2] = 0x03;
    SendBuf[3] = 0x01;
    SendBuf[4] = 0x00;
    SendBuf[5] = 0xFF;


    while(1)
    {
	rtrn = write(uartFd, SendBuf, 6); 
	printf("return Send %d\n", rtrn );
	//sleep(1);
	usleep(500000);	// 500ms
    }

}
int uartParsingReceiveValue(unsigned char* cvalue, int len, unsigned char* remainder, int remainSize )
{

    unsigned char setBuffer[BUFFER_SIZE];
    unsigned char sum = 0;
    int i;
    int offset;

    for( i = 0; i < len; i++ )
    {

	if( cvalue[i] == 0xFE )
	{

	    if( len >= i + (cvalue[i+1] + 3) && ( cvalue[i + (cvalue[i+1] + 2)] == 0xFF ) ) 
	    {
		// 데이터 수신정상
		memcpy( setBuffer, cvalue+i, (cvalue[i+1] + 3) );

		/*
		   for( offset = i; offset < i+(cvalue[i+1] + 3); offset++ )
		   printf("%02X ", cvalue[offset]);
		   printf("\n\n");
		   */


		i += (cvalue[i+1] + 2);

		remainSize = i+1;

		sum = _uartGetCheckSum( setBuffer, i+1 );

		if( sum != setBuffer[(i+1)-2] )
		{
		    //printf("================================\n");
		    //writeLog( "CheckSum Error" );
		}
		else
		{

		    receiveEvent( setBuffer, i+1 );

		}

		sum = 0;

	    }

	}

    }

    remainSize = len - remainSize;
    //printf("reminSize(%d) = len(%d) - remainSize\n", remainSize, len);
    memcpy( remainder, cvalue+(len-(remainSize)), remainSize );


    return remainSize;
}


unsigned char _uartGetCheckSum( unsigned char* buffer, int len )
{
    int intSum = 0;
    unsigned char sum = 0;
    int i;

    //printf("start sum %d\n", len);
    for( i = 1; i< len-2; i++ )
    {
	sum += buffer[i];
	//intSum += buffer[i];
	//sleep(1);
	//printf("%d ", tlvSendBuffer[i]);
    }

    //sum = (unsigned char)intSum;
    //printf("sum %d\n", sum);
    return sum;
}

