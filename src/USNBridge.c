#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <termio.h>
#include <unistd.h>
#include <fcntl.h>
#include <sys/ipc.h>
#include <sys/msg.h>
#include <time.h>
//#include "./include/PointManager.h"
#include "./include/iDCU.h"
#include "./include/CommManager.h"

#include "./include/hiredis/hiredis.h"
#include "./include/hiredis/async.h"
#include "./include/hiredis/adaters/libevent.h"

#include "./include/parser/configInfoParser.h"
#include "./include/debugPrintf.h"
#include "./include/uart.h"


#define LOGPATH "/work/log/USNBridge"
#define CONFIGINFOPATH "/work/config/config.xml"
static CONFIGINFO configInfo;
int rtdid;
int m2mid;
unsigned char _getCheckSum( unsigned char* buffer, int len );


int main(int argc, char** argv)
{
    // usn
    int fd; // Serial Port File Descriptor
    int ReadMsgSize = 0;
    unsigned char DataBuf[BUFFER_SIZE];
    int i;

    unsigned char receiveBuffer[BUFFER_SIZE];
    int receiveSize = 0;
    unsigned char remainder[BUFFER_SIZE];
    int parsingSize = 0;

    int timeoutCount = 0;
    // sqlite
    //int rc;

    // queue
    t_data data[2];
    int cmdLen = 0;
    nodeCMDTLV cmdTlv;

    int test = 0;

    //sleep(6);
    // usn
    fd = OpenCOM();
    if( fd == -2 )
    {
	//writeLog( "Bridge Open Fail." );
	writeLogV2(LOGPATH, "[USNBridge]", "Bridge Open Fail.\n");

	system("/work/script/searchBridge.sh &");

	return -1;
	//exit( 1);
    }
    memset(DataBuf, 0, BUFFER_SIZE);
    memset(receiveBuffer, 0, BUFFER_SIZE);
    memset(remainder, 0, BUFFER_SIZE);

    // queue
    if( -1 == ( rtdid = msgget( (key_t)1111, IPC_CREAT | 0666)))
    {
	//writeLog( "error msgget() CommManager to RealTimeDataManager" );
	writeLogV2(LOGPATH, "[USNBridge]", "error msgget() USNBridge to RealTimeDataManager\n");
	return -1;
    }
    if( -1 == ( m2mid = msgget( (key_t)2222, IPC_CREAT | 0666)))
    {
	//writeLog( "error msgget() CommManager to M2MManager" );
	writeLogV2(LOGPATH, "[USNBridge]", "error msgget() USNBridge to M2MManager\n");
	//perror( "msgget() 실패");
	return -1;
    }

    /************** DB connect.. ***********************/
    // DB Open
    /*
       rc = el1000_sqlite3_open( DBPATH, &pSQLite3 );
    //rc = el1000_sqlite3_open( argv[1], &pSQLite3 );
    if( rc != 0 )
    {
    writeLog( "error DB Open" );
    return -1;
    }
    else
    printf("%s OPEN!!\n", DBPATH);
    //printf("%s OPEN!!\n", argv[1]);

    // DB Customize
    rc = el1000_sqlite3_customize( &pSQLite3 );
    if( rc != 0 )
    {
    writeLog( "error DB Customize" );
    return -1;
    }

    sqlite3_busy_timeout( pSQLite3, 1000);
     */

    //printf("start~~\n");
    while(1)
    { 


	// receive Bridge
	ReadMsgSize = read(fd, DataBuf, BUFFER_SIZE); 
	if(ReadMsgSize > 0)
	{


	    /*
	       for( i = 0; i < ReadMsgSize; i++ )
	       printf("%02X ", DataBuf[i]);
	       printf("\n");

	       */

	    if( receiveSize >= BUFFER_SIZE )
		continue;

	    memcpy( receiveBuffer+receiveSize, DataBuf, ReadMsgSize );
	    receiveSize += ReadMsgSize;

	    parsingSize = ParsingReceiveValue(receiveBuffer, receiveSize, remainder, parsingSize);
	    memset( receiveBuffer, 0 , sizeof(BUFFER_SIZE) );
	    receiveSize = 0;
	    memcpy( receiveBuffer, remainder, parsingSize );
	    receiveSize = parsingSize;

	    parsingSize = 0;	// add 20140508
	    memset( remainder, 0 , sizeof(BUFFER_SIZE) );

	    timeoutCount = 0;
	}
	else
	{
	    if( timeoutCount > 60 )
	    {
		timeoutCount = 0;
		printf("exit USNBridge\n");
		writeLogV2(LOGPATH, "[USNBridge]", "Timeout USNBridge\n");
		return -1;
	    }

	    timeoutCount++;
	    printf("Serial timeout %d(%d)\n", ReadMsgSize, timeoutCount);
	    sleep(1);
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
		cmdLen =  data[M2M_COMMAND].data_buff[1];
		cmdTlv.type =  data[M2M_COMMAND].data_buff[2];
		cmdTlv.length =  data[M2M_COMMAND].data_buff[3];
		memcpy( &cmdTlv.value, data[M2M_COMMAND].data_buff+4, cmdTlv.length );
		//remoteNodeCommand(fd, &cmdTlv );
		remoteNodeCommand(fd, data[M2M_COMMAND].data_buff, data[M2M_COMMAND].data_num );

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

    memcpy( DataBuf, buffer, len );

    size = DataBuf[1]+3;
    type = DataBuf[2];

    if( size != len )
	return -1;

    switch( type )
    {
	case REBOOTNODE:
	    rtrn = write(fd, DataBuf, len); 
	    for(  i = 0; i < rtrn; i++ )
	    {
		printf("%02X ", DataBuf[i]);
	    }
	    printf("send Size %d\n", rtrn);
	    break;

	case CHANGENODEINFO:
	    nodeChangeBuf[4] = DataBuf[4];
	    nodeChangeBuf[5] = DataBuf[5];
	    nodeChangeBuf[7] = DataBuf[6];
	    rtrn = write(fd, nodeChangeBuf, 29); 
	    for(  i = 0; i < rtrn; i++ )
	    {
		printf("%02X ", nodeChangeBuf[i]);
	    }
	    printf("send Size %d\n", rtrn);
	    break;

	case DELETENODE:
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
    //printf("len %d\n", len);

    memset( &data, 0, sizeof( t_data ) );

    data.data_type = 1;   // data_type 는 1
    //sprintf( data.data_buff, "%s", buffer);
    //memcpy( data.data_buff, buffer, len );
    data.data_buff[offset++] = buffer[NODEID];
    data.data_buff[offset++] = buffer[GROUP];
    data.data_buff[offset++] = buffer[SEQ];

    data.data_buff[offset++] = buffer[WO];
    data.data_buff[offset++] = buffer[WO+3];
    data.data_buff[offset++] = buffer[WO+6];
    data.data_buff[offset++] = buffer[WO+9];
    data.data_buff[offset++] = buffer[WO+12];
    data.data_buff[offset++] = buffer[WO+15];

    for( i = 0; i < DI_PORT; i++ )
    {
	data.data_buff[offset++] = buffer[CNT+cntOffset++];
	data.data_buff[offset++] = buffer[CNT+cntOffset++];
	data.data_buff[offset++] = buffer[CNT+cntOffset++];
	data.data_buff[offset++] = buffer[CNT+cntOffset++];
	cntOffset += 2;
    }


    for( i = 0; i < offset; i++ )
    {
	data.data_buff[offset] += data.data_buff[i];
	//printf("%02X ", data.data_buff[i]);
    }
    //printf("\n");

    data.data_num = ++offset; 
    /*
       printf("check sum = %d, length = %d\n",
       data.data_buff[offset-1],
       data.data_num
       );
     */

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

    //printf("\n");

    for( i = 0; i < len; i++ )
    {



	if( cvalue[i] == 0xFE )
	{

	    /*
	       printf("stx i = %02x, len = %d\n", i, len);
	       for( offset = i; offset < len; offset++ )
	       printf("%02X ", cvalue[offset]);
	       printf("\n");
	     */
	    /*
	       printf("cvalue[i + 40] = %02X\n", cvalue[i + 40] );
	       printf("cvalue[i + 83] = %02X\n", cvalue[i + 83] );
	       printf("cvalue[i + 84] = %02X\n", cvalue[i + 84] );
	       printf("cvalue[i + 85] = %02X\n", cvalue[i + 85] );
	     */
	    //cvalue[i + 84]
	    if( len >= i + 85 && ( cvalue[i + 84] == 0xFF ) )
	    {
		/*
		   printf("etx\n");
		   for( offset = i; offset < i+85; offset++ )
		   printf("%02X ", cvalue[offset]);
		   printf("\n");
		 */


		// 데이터 수신정상
		memcpy( setBuffer, cvalue+i, 85 );

		/*
		   for( offset = i; offset < i+85; offset++ )
		   printf("%02X ", cvalue[offset]);
		   printf("\n");

		   InsertDebugNodeData( &pSQLite3, setBuffer );
		 */

		i += 84;

		remainSize = i+1;
		sum = _getCheckSum( setBuffer, i+1 );

		/*
		   printf("%02X == %02X\n", sum, setBuffer[(i+1)-2] );
		   for( offset = 0; offset < 85; offset++ )
		   printf("%02X ", setBuffer[offset]);
		   printf("\n");
		 */


		if( sum != setBuffer[(i+1)-2] )
		{

		    //printf("================================\n");

		    //writeLog( "CheckSum Error" );
		}
		else
		{
		    selectTag( setBuffer, i+1 );
		}

		sum = 0;

	    }

	}

    }

    remainSize = len - remainSize;
    memcpy( remainder, cvalue+(len-(remainSize)), remainSize );


    return remainSize;
}
int OpenCOM(void)
{
    int fd; // File descriptor
    struct termios newtio;

    int rtrn;
    int	dev;
    char	 buff[32];
    char 	*devName;

    memset( buff, 0, sizeof( buff ) );

    dev = open( "/work/config/devName", O_RDONLY);
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
    fd = open( devName, O_RDWR | O_NOCTTY);


    debugPrintf(configInfo.debug, "devName %s\n", devName);

    /*
       fd = OpenSerialV2( devName, "115200", 'N', '8', '1');

       free( devName );	

       if(fd < 0)
       {
       printf("Bridge Open Fail.\n");
       return -2;
       }
     */

    if(fd < 0)
    {
	printf("Bridge Open Fail.\n");
	return -2;
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

    free( devName );	

    return fd;
}

int CloseCOM(int fd)
{
    close(fd);
}

