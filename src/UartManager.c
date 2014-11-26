#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <termio.h>
#include <unistd.h>
#include <fcntl.h>
#include <sys/ipc.h>
#include <sys/msg.h>
#include <time.h>
#include <pthread.h>

#include "./include/iDCU.h"
#include "./include/sqlite3.h"
#include "./include/CommManager.h"
#include "./include/writeLog.h"
#include "./include/sqliteManager.h"

int rtdid;
int m2mid;
pthread_t threads[5];
int done[5];

	sqlite3 *pSQLite3;

void *thread_main(void *);
unsigned char _getCheckSum( unsigned char* buffer, int len );

typedef struct CNTVALUE {

    unsigned char type;
    unsigned char length;
    unsigned char value[4]; 

} CNTVALUE;

CNTVALUE di[DI_PORT];


int main(int argc, char** argv)
{
    // usn
    int ReadMsgSize = 0;
    unsigned char DataBuf[BUFFER_SIZE];
    int i;

    unsigned char receiveBuffer[BUFFER_SIZE];
    int receiveSize = 0;
    unsigned char remainder[BUFFER_SIZE];
    int parsingSize = 0;


    int fd; // Serial Port File Descriptor
    // sqlite...
    //int rc;

    // queue
    t_data data[2];
    t_data data2;
    int cmdLen = 0;
    nodeCMDTLV cmdTlv;

    int test = 0;
    int rc;
    int status;



    // usn
    fd = OpenSerial();
    if( fd == -1 )
    {
	writeLog("/work/comm/log", "Uart Open Fail." );
	return -1;
    }
    else if ( fd == -2 )
    {
	return -1;
    }
    memset(DataBuf, 0, BUFFER_SIZE);
    memset(receiveBuffer, 0, BUFFER_SIZE);
    memset(remainder, 0, BUFFER_SIZE);

    char th_data[256];
    memcpy( th_data, (void *)&fd, sizeof(fd) );
    // thread
    if( pthread_create(&threads[0], NULL, &thread_main, (void *)th_data) == -1 )
    {
	//perror("thread Create error\n");
	writeLog("/work/comm/log", "[UartManager] thread Create error" );
    }

    // queue
    if( -1 == ( rtdid = msgget( (key_t)1111, IPC_CREAT | 0666)))
    {
	writeLog("/work/comm/log", "[UartManager] error msgget() CommManager to RealTimeDataManager" );
	return -1;
    }
    if( -1 == ( m2mid = msgget( (key_t)2222, IPC_CREAT | 0666)))
    {
	writeLog("/work/comm/log", "[UartManager] error msgget() CommManager to M2MManager" );
	//perror( "msgget() 실패");
	return -1;
    }

    printf("start~~\n");

    /************** DB connect.. ***********************/
    // DB Open
    rc = _sqlite3_open( "/work/db/comm", &pSQLite3 );
    //rc = el1000_sqlite3_open( argv[1], &pSQLite3 );
    if( rc != 0 )
    {
	    writeLog( "/work/comm/log",  "error DB Open" );
	    return -1;
    }
    else
    {
	    writeLog( "/work/comm/log",  "DB Open" );
    }

    // DB Customize
    rc = _sqlite3_customize( &pSQLite3 );
    if( rc != 0 )
    {
	    writeLog( "/work/comm/log",  "error DB Customize" );
	    return -1;
    }
    rc = _sqlite3_nolock( &pSQLite3 );
    if( rc != 0 )
    {
	writeLog( "/work/comm/log", "error set nolock" );
	return -1;
    }


    printf("go~~\n");
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

	    parsingSize = ParsingReceiveValue(receiveBuffer, receiveSize, remainder, parsingSize);

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
	    usleep(100);
	    printf("Serial timeout %d\n", ReadMsgSize);
	}


	// receive Command
	if( -1 == msgrcv( m2mid, &data[M2M_COMMAND], sizeof( t_data) - sizeof( long), 1, IPC_NOWAIT) )
	{
	    //printf("Empty Q\n");
	}
	else
	{
	    printf( "Recv %d byte\n", data[M2M_COMMAND].data_num );
	    for( test = 0; test < data[M2M_COMMAND].data_num; test++ )
		printf("%02X ", data[M2M_COMMAND].data_buff[test]);
	    printf("\n");

	    if( data[M2M_COMMAND].data_buff[0] == 0xFE && data[M2M_COMMAND].data_buff[data[M2M_COMMAND].data_num-1] == 0xFF)
	    {
		/*
		   cmdLen =  data[M2M_COMMAND].data_buff[1];
		   cmdTlv.type =  data[M2M_COMMAND].data_buff[2];
		   cmdTlv.length =  data[M2M_COMMAND].data_buff[3];
		   memcpy( &cmdTlv.value, data[M2M_COMMAND].data_buff+4, cmdTlv.length );
		 */
		//remoteNodeCommand(fd, &cmdTlv );
		//remoteNodeCommand(fd, data[M2M_COMMAND].data_buff, data[M2M_COMMAND].data_num );
		memset( &data2, 0, sizeof( t_data ) );

		data2.data_type = 2;   // data_type ´Â 1
		memcpy( data2.data_buff, &di, sizeof(CNTVALUE)*DI_PORT );
		data2.data_num = sizeof(CNTVALUE)*DI_PORT; 

		for( i = 0; i < data2.data_num; i++ )
		{
		    printf("%02X ", data2.data_buff[i]);
		}
		printf("\n");

		if ( -1 == msgsnd( m2mid, &data2, sizeof( t_data) - sizeof( long), IPC_NOWAIT))
		{
		    perror( "msgsnd() 실패");
		}


		/*
		   for( i = 0; i < cmdLen+3; i++ ) 
		   printf("%02X ",  data[M2M_COMMAND].data_buff[i]);
		   printf("\n");
		 */
	    }
	    /*
	       data[USN_RETURN].data_type = 2;   // data_type 는 2
	       data[USN_RETURN].data_num = data[M2M_COMMAND].data_num;
	       sprintf( data[USN_RETURN].data_buff, "type=%d, ndx=%d, USN Return Msg", data[USN_RETURN].data_type, data[USN_RETURN].data_num);
	       if ( -1 == msgsnd( m2mid, &data[USN_RETURN], sizeof( t_data) - sizeof( long), IPC_NOWAIT))
	       {
	       perror( "msgsnd() 실패");
	    //exit( 1);
	    }
	     */
	}

    }

    CloseSerial(fd);

    return 0;
}

void *thread_main(void *arg)
{

    int rtrn;
    int uartFd;
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
	usleep(200);
	rtrn = write(uartFd, SendBuf, 6); 
	//printf("Send %d\n", rtrn );
    }

}
//int remoteNodeCommand(int fd, nodeCMDTLV *cmdTlv)
int remoteNodeCommand(int fd, unsigned char *buffer, int len)
{

    int i = 0;
    int rtrn; 
    unsigned char DataBuf[len];
    unsigned char nodeChangeBuf[29] = {0xFE,
	0x1A,
	0x1E, 0x18, 0x07,
	0x07, 0x00, 0x04, 0x1A,
	0x04, 0x00,
	0x00, 0x32, 0x00, 0x32, 0x00, 0x32, 0x00, 0x32, 0x00, 0x32, 0x00, 0x32,
	0x01, 0x01, 0x01, 0x01, 0x01,
	0xFF };

    unsigned char type;
    unsigned char size;

    t_data data;

    memcpy( DataBuf, buffer, len );

    size = DataBuf[1]+3;
    type = DataBuf[2];

    if( size != len )
	return -1;

    switch( type )
    {
	case REBOOTNODE:
	    rtrn = write(fd, DataBuf, len); 
	    printf("rtrn %d\n", rtrn);
	    break;

	case CHANGENODEINFO:
	    nodeChangeBuf[4] = DataBuf[4];
	    nodeChangeBuf[5] = DataBuf[5];
	    nodeChangeBuf[7] = DataBuf[6];
	    rtrn = write(fd, nodeChangeBuf, 29); 
	    printf("rtrn %d\n", rtrn);
	    break;

	case DELETENODE:
	    break;

	case GETCNT:


	    memset( &data, 0, sizeof( t_data ) );

	    data.data_type = 2;   // data_type ´Â 1
	    memcpy( data.data_buff, &di, sizeof(CNTVALUE)*DI_PORT );
	    data.data_num = sizeof(CNTVALUE)*DI_PORT; 

	    for( i = 0; i < data.data_num; i++ )
	    {
		printf("%02X ", data.data_buff[i]);
	    }
	    printf("\n");

	    if ( -1 == msgsnd( m2mid, &data, sizeof( t_data) - sizeof( long), IPC_NOWAIT))
	    {
		perror( "msgsnd() 실패");
	    }

	    break;
    }
    /*
       printf("Type %d\n", cmdTlv->type );
       printf("Length %d\n", cmdTlv->length );

       printf("Value ");
       for( i = 0; i < cmdTlv->length; i++ )
       printf("%d ", cmdTlv->value[i]);
       printf("\n");
       switch( cmdTlv->type )
       {
       case 1:

       break;
       }
     */

    return 0;
}

int selectTag(unsigned char* buffer, int len )
{

    t_data data;
    t_node node;
    int i;
    int offset = 0;
    int cntOffset = 0;
    printf("len %d\n", len);


    data.data_type = 1;   // data_type 는 1
    data.data_num = 0; 
    //sprintf( data.data_buff, "%s", buffer);
    //memcpy( data.data_buff, buffer, len );

    cntOffset = 4;
    for( i = 0; i < DI_PORT; i++ )
    {
	data.data_buff[offset++] = buffer[cntOffset++];
	data.data_buff[offset++] = buffer[cntOffset++];
	data.data_buff[offset++] = buffer[cntOffset++];
	data.data_buff[offset++] = buffer[cntOffset++];
	cntOffset += 2;
    }

    for( i = 0; i < offset; i++ )
    {
	printf("%02X ", data.data_buff[i]);
    }
    printf("\n\n");

    if ( -1 == msgsnd( rtdid, &data, sizeof( t_data) - sizeof( long), IPC_NOWAIT))
    {
	//perror( "msgsnd() error ");
	//writeLog( "msgsnd() error : Queue full" );
	sleep(1);
	return -1;
    }

    return 0;
}

unsigned char _getCheckSum( unsigned char* buffer, int len )
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

int ParsingReceiveValue(unsigned char* cvalue, int len, unsigned char* remainder, int remainSize )
{

    unsigned char setBuffer[BUFFER_SIZE];
    unsigned char sum = 0;
    int i;
    int offset;
    int cntOffset;
    MGData node;


    for( i = 0; i < len; i++ )
    {

	//printf("%02X ", cvalue[i]);



	if( cvalue[i] == 0xFE )
	{

	    //printf("match STX  len =%d\n", len);
	    //printf("remainSize %d~~~\n", remainSize);
	    //if( len >= i + 52 && ( cvalue[i + 51] == 0xFF ) ) 
	    if( len >= i + (cvalue[i+1] + 3) && ( cvalue[i + (cvalue[i+1] + 2)] == 0xFF ) ) 
	    {
		// 데이터 수신정상
		memcpy( setBuffer, cvalue+i, (cvalue[i+1] + 3) );

		/*
		for( offset = i; offset < i+(cvalue[i+1] + 3); offset++ )
		    printf("%02X ", cvalue[offset]);
		printf("\n\n");

		*/
		/*
		   InsertDebugNodeData( &pSQLite3, setBuffer );
		 */

		i += (cvalue[i+1] + 2);

		remainSize = i+1;
		sum = _getCheckSum( setBuffer, i+1 );

		if( sum != setBuffer[(i+1)-2] )
		{
		    //printf("================================\n");
		    //writeLog( "CheckSum Error" );
		}
		else
		{
		    //printf("OOK\n");

		    cntOffset = 4;
		    for( i = 0; i < 8; i++ )
		    {
			node.cnt[i] = 0;

			node.cnt[i] = setBuffer[cntOffset++];
			node.cnt[i] |= setBuffer[cntOffset++];
			node.cnt[i] |= setBuffer[cntOffset++];
			node.cnt[i] |= setBuffer[cntOffset++];

			cntOffset += 2;
			
			//printf("[%d]  %ld ", i, node.cnt[i] );
		    }
		    //printf("\n");


		    _sqlite3_update( &pSQLite3, node );
	
		    //selectTag( setBuffer, i+1 );
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

int OpenSerial(void)
{
    int fd; // File descriptor
    struct termios newtio;

    int rtrn;
    int	dev;
    char	 buff[32];
    char 	*devName;

    memset( buff, 0, sizeof( buff ) );

    dev = open( "/work/comm/devName", O_RDONLY);
    if(dev < 0)
    {
	printf("devName Open Fail.\n");
	return -2;
    }

    rtrn = read( dev, buff, 32 );
    buff[rtrn-1] = '\0';

    devName = (char*)malloc(rtrn);
    memcpy( devName, buff, rtrn );

    close( dev );

    //fd = open(USN, O_RDWR | O_NOCTTY);
    fd = open( "/dev/ttyAMA0", O_RDWR | O_NOCTTY);

    free( devName );	

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

