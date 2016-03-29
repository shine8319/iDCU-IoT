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
#include "./include/sqliteManager.h"
#include "./include/M2MManager.h"
#include "./include/configRW.h"
#include "./include/writeLog.h"
#include "./include/networkInfo.h"
#include "./include/UpgradeFirmware.h"
#include "./include/DirectInterface.h"
//#include "./include/UDPConnect.h"

#include "./include/debugPrintf.h"
#include "./include/uart.h"

#include "./include/parser/serialInfoParser.h"
#include "./include/parser/configInfoParser.h"
#include "./include/parser/timeSyncInfoParser.h"

#include "./include/hiredis/hiredis.h"
#include "./include/hiredis/async.h"
#include "./include/hiredis/adaters/libevent.h"

#include "./include/RedisControl.h"

#define LOGPATH "/work/log/SetupManager"
#define DBPATH "/work/db/comm"
#define WPAPATH "/work/script/network/wlan/security/wpa_supplicant.conf"
#define SERIALINFOPATH "/work/config/serial_info.xml"
#define CONFIGINFOPATH "/work/config/config.xml"
#define ETH0INFOPATH "/work/config/eth0.config"
#define WLAN0INFOPATH "/work/config/wlan0.config"
#define SECURITYINFOPATH "/work/config/security.config"
#define NETWORKUSEINFOPATH  "/work/config/networkUse.config"
#define OPTIONSINFOPATH "/work/config/options.config"
#define TIMESYNCINFOPATH "/work/config/timeSync.xml"
#define DIRECTINTERFACEINFOPATH "/work/config/directInterface.xml"	// add 2016.03.22 for machine direct interface test


#define SQLITE_SAFE_FREE(x)	if(x){ x = NULL; }
#define	BUFF_SIZE 1024

unsigned char user_quit = 0;


static CONFIGINFO configInfo;

static int getPointInfo( void );

int Socket_Manager( int *fd);
int Pack_Manager( unsigned char* buffer, int len );

int UDPConnect( int port );
void *thread_main(void *);

void command_GetDirectInterfaceInfo();
void command_SetDirectInterfaceInfo(unsigned char* buffer);

static redisContext *c;


struct sockaddr_in   client_addr;
int client_addr_size;
int udp;


DEVINFO	    device;
DEVINFO	    saveDev;


sqlite3 *pSQLite3;

int client_sock; 

void quitsignal(int sig) 
{
    printf("Press <ctrl+c> to user_quit. \n");
    user_quit = 1;
    close(udp);
    exit(1);
}

static int getPointInfo( void ) {

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
		//pastData.cnt[j] = atoi(data.data[rowSize++]);
		//node.cnt[j] = pastData.cnt[j];
		//IO[j].cnt = pastData.cnt[j];
		device.point[j].port = j;
		device.point[j].value = atoi(data.data[rowSize++]);

	    }

	}
    }
    else
	return -1;


    return 0;
}



int main(int argc, char **argv) { 

    int i;
    pthread_t p_thread;
    int thr_id;
    int status;

    int rc;

    void *socket_fd;
    //int socket_fd;
    struct sockaddr_in servaddr; //server addr

    (void)signal(SIGINT, quitsignal);

    configInfo = configInfoParser(CONFIGINFOPATH);
    c = redisInitialize();
    /******************** DB connect ***********************/
    //char	*szErrMsg;
    // DB Open
    rc = _sqlite3_open( DBPATH, &pSQLite3 );
    if( rc != 0 )
    {
	writeLogV2(LOGPATH, "[SetupManager]", "error DB Open\n");
	return -1;
    }
    else
    {
	writeLogV2(LOGPATH, "[SetupManager]", "DB Open\n");
    }


    // DB Customize
    rc = _sqlite3_customize( &pSQLite3 );
    if( rc != 0 )
    {
	writeLogV2(LOGPATH, "[SetupManager]", "error DB Customize\n");
	return -1;
    }

    getPointInfo();

    /***************** Server Connect **********************/
    configInfo = configInfoParser(CONFIGINFOPATH);
    if( pthread_create(&p_thread, NULL, &thread_main, NULL) == -1 )
    {
	writeLogV2(LOGPATH, "[SetupManager]", "create thread_main error\n");
	return -1;
    }


    //Connect_Manager();
    udp = UDPConnect( atoi(configInfo.setupport) );
    writeLogV2(LOGPATH, "[SetupManager]", "UDPConnect %d\n", udp);
    Socket_Manager( &udp ); 

    /*******************************************************/
    _sqlite3_close( &pSQLite3 );

    writeLogV2(LOGPATH, "[SetupManager]", "End");
    close(udp);

    return 0; 
} 

int UDPConnect( int port )
{
    int   sock;
    //int   client_addr_size;
    int   rtrn;
    int broadcastEnable = 1;

    struct sockaddr_in   server_addr;
    //struct sockaddr_in   client_addr;

    //char   buff_rcv[BUFF_SIZE+5];
    //char   buff_snd[BUFF_SIZE+5];


    //client_addr_size  = sizeof( client_addr);
    sock  = socket( AF_INET, SOCK_DGRAM, 0);

    if( -1 == sock)
    {
	printf( "socket error\n");
	exit( 1);
    }

    /*
    // add by hyungoo.kang
    // solution ( bind error : Address already in use )
    if( setsockopt( server_sock, SOL_SOCKET, SO_REUSEADDR, (void*)&bufsize, (socklen_t)rn) < 0 ) {
    perror("Error setsockopt()");
    return;
    }
     */


    //rtrn = setsockopt(sock, SOL_SOCKET, SO_BROADCAST, &broadcastEnable, sizeof(broadcastEnable));
    rtrn = setsockopt(sock, SOL_SOCKET, SO_REUSEADDR, &broadcastEnable, sizeof(broadcastEnable));
    if( rtrn == -1 )
    {
	printf( "setsockopt error\n");
	exit( 1);
    }


    memset( &server_addr, 0, sizeof( server_addr));

    server_addr.sin_family     = AF_INET;
    server_addr.sin_port       = htons( port );
    server_addr.sin_addr.s_addr= htonl( INADDR_ANY);
    //server_addr.sin_addr.s_addr= htonl( INADDR_BROADCAST );

    if( -1 == bind( sock, (struct sockaddr*)&server_addr, sizeof( server_addr) ) )
    {
	printf( "bind() error\n");
	exit( 1);
    }

    /*
       while( 1)
       {
       memset( buff_rcv, 0, BUFF_SIZE+5 );
       memset( buff_snd, 0, BUFF_SIZE+5 );

       rtrn = recvfrom( sock, buff_rcv, BUFF_SIZE, 0 , ( struct sockaddr*)&client_addr, &client_addr_size);
       printf("from %s(%d) receive: %s\n", inet_ntoa(client_addr.sin_addr), ntohs(client_addr.sin_port), buff_rcv);

       sprintf( buff_snd, "return Msg : %s", buff_rcv);
       rtrn = sendto( sock, buff_snd, strlen( buff_snd)+1, 0,( struct sockaddr*)&client_addr, sizeof( client_addr));
       printf("return %d\n", rtrn);


       }
     */
    return sock;
}
int Socket_Manager( int *fd) {

    unsigned char DataBuf[BUFFER_SIZE];
    int ReadMsgSize;

    unsigned char receiveBuffer[BUFFER_SIZE];
    int receiveSize = 0;
    unsigned char remainder[BUFFER_SIZE];
    int parsingSize = 0;

    int rtrn;
    int i;
    int nd;

    writeLogV2(LOGPATH, "[Socket_Manager]", "Start");

    client_addr_size  = sizeof( client_addr);

    while( !user_quit ) 
    {

	//printf("go\n");
	//sleep(10);
	//ReadMsgSize = recvfrom( *fd, DataBuf, BUFFER_SIZE, 0 , ( struct sockaddr*)&client_addr, &client_addr_size);
	ReadMsgSize = recvfrom( udp, DataBuf, BUFFER_SIZE, 0 , ( struct sockaddr*)&client_addr, &client_addr_size);
	if( ReadMsgSize > 0 ) 
	{

	    if( ReadMsgSize >= BUFFER_SIZE )
		continue;

	    printf("from %s(%d) receive data size : %d\n", inet_ntoa(client_addr.sin_addr), ntohs(client_addr.sin_port), ReadMsgSize);

	    for( i = 0; i < ReadMsgSize; i++ ) {
		printf("%-3.2x", DataBuf[i]);
	    }
	    printf("\n");


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
	}
	ReadMsgSize = 0;
    }	// end of while

    writeLogV2(LOGPATH, "[Socket_Manager]", "End");
    return 0;

}


int ParsingReceiveValue(unsigned char* cvalue, int len, unsigned char* remainder, int remainSize )
{

    unsigned char setBuffer[BUFFER_SIZE];
    int i;
    int offset;
    int totalSize = 0;;

    for( i = 0; i < len; i++ )
    {

	if( cvalue[i] == 0xFE )
	{

	    totalSize = cvalue[i+1] << 8;
	    totalSize |= cvalue[i+2] << 0;

	    //if( len >= i + cvalue[i+1] && ( cvalue[i + (cvalue[i+1]+2)] == 0xFF ) )
	    if( len >= i + totalSize && ( cvalue[i + (totalSize+3)] == 0xFF ) )
	    {
		//memcpy( setBuffer, cvalue+i, cvalue[i+1]+3 );
		memcpy( setBuffer, cvalue+i, totalSize+4 );

		//i += (cvalue[i+1]+2);
		i += (totalSize+3);

		remainSize = i+1;

		Pack_Manager( setBuffer, i+1 );

	    }

	}

    }

    remainSize = len - remainSize;
    memcpy( remainder, cvalue+(len-(remainSize)), remainSize );


    return remainSize;
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

    //type = buffer[2];
    //nodeid = buffer[4];
    type = buffer[3];
    nodeid = buffer[5];

    SERIALINFO xmlinfo;
    TIMESYNCINFO timeSyncInfo;

    redisReply *reply;
    UINT8 point;


    switch( type )
    {
	case TIMESYNC:
	    break;
	case GETNODELIST:
	    break;
	case DELETENODE:
	    break;
	case CHANGENODEINFO:
	    break;
	case REBOOTNODE:
	    execl("/sbin/reboot", "/sbin/reboot", NULL);
	    break;

	case GETDRIVERLIST:

	    memset( sendBuffer, 0, sizeof( sendBuffer) );
	    vLength = getDriverList(sendBuffer);
	    totalLength = vLength + 3;	// type 1byte, length 2byte

	    sendBuffer[0] = 0xFE;
	    sendBuffer[1] = totalLength >> 8;	// total length;
	    sendBuffer[2] = totalLength;	// total length;
	    sendBuffer[3] = GETDRIVERLIST;
	    sendBuffer[4] = vLength >> 8;
	    sendBuffer[5] = vLength;

	    sendBuffer[totalLength+3] = 0xFF;
	    rtrn = sendto( udp, sendBuffer, totalLength+4, 0,( struct sockaddr*)&client_addr, sizeof( client_addr));
	    break;

	case GETDIRECTINTERFACEINFO:
	    command_GetDirectInterfaceInfo();
	    break;

	case SETDIRECTINTERFACEINFO:

	    printf("SETDIRECTINTERFACEINFO\n");
	    command_SetDirectInterfaceInfo(buffer);
	    break;


	case SETCNT:
	    point = buffer[6];
	    reply = redisCommand(c,"PUBLISH clear %d",   point+34);
	    printf("type:%d / index:%lld\n", reply->type, reply->integer);
	    printf("len:%d / str:%s\n", reply->len, reply->str);
	    printf("elements:%d\n", reply->elements );

	    break;

	case GETLANINFO:

	    memset( sendBuffer, 0, sizeof( sendBuffer) );

	    vLength = sizeof( DEVINFO );
	    totalLength = vLength + 3;	// type 1byte, length 2byte

	    sendBuffer[0] = 0xFE;
	    sendBuffer[1] = totalLength >> 8;	// total length;
	    sendBuffer[2] = totalLength;	// total length;
	    sendBuffer[3] = 0x06;
	    sendBuffer[4] = vLength >> 8;
	    sendBuffer[5] = vLength;

	    ReadNetworkConfig(ETH0INFOPATH, &device.lan);
	    ReadNetworkConfig(WLAN0INFOPATH, &device.wlan);
	    ReadSecurityConfig(SECURITYINFOPATH, &device.security);

	    ReadNetworkUseConfig(NETWORKUSEINFOPATH, &device.lanEnable, &device.wlanEnable );

	    ReadOptionsConfig(OPTIONSINFOPATH, &device.lanAuto, &device.wlanAuto);

	    // serial tab 2015.05.15
	    xmlinfo = serialInfoParser(SERIALINFOPATH);
	    device.comport.baud = getBaudrate(xmlinfo.comport.baud);
	    device.comport.parity = getParity(xmlinfo.comport.parity[0]);	// none
	    device.comport.databit = getDatabit(xmlinfo.comport.databit[0]);
	    device.comport.stopbit = getStopbit(xmlinfo.comport.stopbit[0]);	// 1 
	    device.comport.flowcontrol = 0;	// none

	    device.connect.mode = getConnectMode(xmlinfo.connect.mode);
	    device.connect.ip = inet_addr( xmlinfo.connect.ip );
	    device.connect.port = atoi( xmlinfo.connect.port );
	    device.connect.localport = atoi( xmlinfo.connect.localport ); 
	    device.connect.timeout = atoi( xmlinfo.connect.timeout );

	    unsigned char addr[4] = {0,};
	    char convertAddr[14];

	    //if( device.lanEnable && device.lanAuto )
	    {
		memset( convertAddr, 0, 14);

		s_getIpAddress("eth0", addr);
		printf("ip addr:=%d.%d.%d.%d\n", (int)addr[0], (int)addr[1], (int)addr[2], (int)addr[3]);
		sprintf( convertAddr, "%d.%d.%d.%d", (int)addr[0], (int)addr[1], (int)addr[2], (int)addr[3]);
		device.lan.lan_addr = inet_addr( convertAddr );
		printf("inet_addr = %ld\n", device.lan.lan_addr );
	    }
	    //if( device.wlanEnable && device.wlanAuto )
	    {
		memset( convertAddr, 0, 14);

		s_getIpAddress("wlan0", addr);
		printf("ip addr:=%d.%d.%d.%d\n", (int)addr[0], (int)addr[1], (int)addr[2], (int)addr[3]);
		sprintf( convertAddr, "%d.%d.%d.%d", (int)addr[0], (int)addr[1], (int)addr[2], (int)addr[3]);
		device.wlan.lan_addr = inet_addr( convertAddr );
		printf("inet_addr = %ld\n", device.wlan.lan_addr );
	    }

	    // get Digital Count
	    getPointInfo();

	    configInfo = configInfoParser(CONFIGINFOPATH);
	    // digital input check interval 2015.08.12
	    for( i = 0; i < 8; i++ )
	    {
		device.config.interval[i] =  atoi(configInfo.port[i].interval);
		printf("%d | ", device.config.interval[i]);
	    }
	    printf("\n");

	    device.config.setupport =  atoi(configInfo.setupport);
	    printf("udp %ld\n", device.config.setupport);

	    // timeSync tab 2015.06.04
	    timeSyncInfo = timeSyncInfoParser(TIMESYNCINFOPATH);
	    device.timeSync.cycle = getTimeSyncCycle(timeSyncInfo.cycle);
	    device.timeSync.address = inet_addr( timeSyncInfo.address );

	    // version info 2015.08.03
	    unsigned char mac[6] = {0,};
	    char convertMac[17];

	    s_getMacAddress("eth0", mac);
	    printf("mac addr:=%02X:%02X:%02X:%02X:%02X:%02X\n", mac[0], mac[1], mac[2], mac[3],mac[4], mac[5]);
	    sprintf( convertMac, "%02X:%02X:%02X:%02X:%02X:%02X", mac[0], mac[1], mac[2], mac[3],mac[4], mac[5]);

	    memcpy( device.version.mac, convertMac, 17 );


	    char ver[16] = {0,};
	    int rtn = s_getFirmware(ver);
	    if( rtn == 0 )
		sprintf( device.version.ver, "x:x:x");
	    else
		sprintf( device.version.ver, "%s", ver);


	    memcpy( sendBuffer+6, &device, vLength );
	    sendBuffer[totalLength+3] = 0xFF;


	    rtrn = sendto( udp, sendBuffer, totalLength+4, 0,( struct sockaddr*)&client_addr, sizeof( client_addr));
	    printf("Send GETLANINFO %d\n", rtrn);


	    for( i = 0; i < rtrn; i++ )
		printf("%02X ", sendBuffer[i]);
	    printf("\n");
	    break;
	case SETLANINFO:

	    memset( sendBuffer, 0, sizeof( sendBuffer) );
	    memcpy( &saveDev, buffer+6, sizeof( DEVINFO ) );	// data start offset 6

	    vLength = 2;
	    totalLength = vLength + 3;	// type 1byte, length 2byte

	    sendBuffer[0] = 0xFE;
	    sendBuffer[1] = totalLength >> 8;	// total length;
	    sendBuffer[2] = totalLength;	// total length;
	    sendBuffer[3] = 0x0A;
	    sendBuffer[4] = vLength >> 8;
	    sendBuffer[5] = vLength;
	    sendBuffer[6] = 0x0B;
	    sendBuffer[7] = 0x00;


	    WriteNetworkUseConfig(NETWORKUSEINFOPATH, &saveDev.lanEnable, &saveDev.wlanEnable );
	    WriteOptionsConfig(OPTIONSINFOPATH, &saveDev.lanAuto, &saveDev.wlanAuto);

	    if(saveDev.lanEnable )
	    {
		WriteNetworkConfig(ETH0INFOPATH, &saveDev.lan);
		if( !saveDev.lanAuto )
		{
		    rtrn = system("/work/script/network/lan/eth0.sh &");
		    printf("system() %d\n", rtrn);

		    //if( rtrn != 0 )
		    //sendBuffer[7] = 0x01;
		}
		else
		    rtrn = system("/work/script/network/lan/eth0Static.sh &");

	    }

	    if( saveDev.wlanEnable )
	    {
		WriteNetworkConfig(WLAN0INFOPATH, &saveDev.wlan);
		WriteSecurityConfig(SECURITYINFOPATH, &saveDev.security);
		WriteSecurityScript(WPAPATH, &saveDev.security);

		if( !saveDev.wlanAuto )
		{
		    rtrn = system("/work/script/network/wlan/wlan0.sh &");
		    printf("system() %d\n", rtrn);

		    //if( rtrn != 0 )
		    //sendBuffer[7] = 0x01;
		}
		else
		    rtrn = system("/work/script/network/wlan/wlan0Static.sh &");

	    }

	    WriteSerialConfig( SERIALINFOPATH, &saveDev.comport, &saveDev.connect );
	    WriteTimeSyncConfig( TIMESYNCINFOPATH, &saveDev.timeSync);
	    //add 2015.08.12
	    WriteConfigConfig( CONFIGINFOPATH, &saveDev.config);

	    rtrn = system("/work/script/restartProgram.sh &");

	    sendBuffer[totalLength+3] = 0xFF;


	    rtrn = sendto( udp, sendBuffer, totalLength+4, 0,( struct sockaddr*)&client_addr, sizeof( client_addr));
	    printf("SET INFO %d\n", rtrn);


	    break;


    }

    return 0;

}

// call in Pack_Manager case GETDIRECTINTERFACEINFO:
void command_GetDirectInterfaceInfo()
{
    unsigned char sendBuffer[1024];

    int totalLength = 0;
    int vLength = 0;
    int rtrn,i = 0;
    COMMAND_0X16_INFO pack;

    memset( sendBuffer, 0, sizeof( sendBuffer ) );
    memset( &pack, 0, sizeof( COMMAND_0X16_INFO ) );

    DIRECTINTERFACEINFO info = getDirectInterfaceInfo(DIRECTINTERFACEINFOPATH);
    printf("end parser\n");

    pack.driverId = atoi( info.driverId );
    pack.baseScanRate = atoi( info.baseScanRate );
    pack.slaveId = atoi( info.slaveId );
    pack.function = atoi( info.function );
    pack.address = atoi( info.address );
    pack.offset = atoi( info.offset);

    pack.host = inet_addr( info.host );
    pack.port = atoi( info.port );

    memcpy( pack.auth, info.auth, strlen(info.auth) );
    pack.db = atoi( info.db );
    memcpy( pack.key, info.key, strlen(info.key) );

    vLength = sizeof(COMMAND_0X16_INFO);
    totalLength = vLength + 3;	// type 1byte, length 2byte

    sendBuffer[0] = 0xFE;
    sendBuffer[1] = totalLength >> 8;	// total length;
    sendBuffer[2] = totalLength;	// total length;
    sendBuffer[3] = GETDIRECTINTERFACEINFO;	// 0x16
    sendBuffer[4] = vLength >> 8;
    sendBuffer[5] = vLength;

    memcpy( sendBuffer+6, &pack, vLength );

    sendBuffer[totalLength+3] = 0xFF;

    rtrn = sendto( udp, sendBuffer, totalLength+4, 0,( struct sockaddr*)&client_addr, sizeof( client_addr));

    for( i = 0; i < rtrn; i++ )
	printf("%02X ", sendBuffer[i]);
    printf("\n");
   
}

// call in Pack_Manager case SETDIRECTINTERFACEINFO:
void command_SetDirectInterfaceInfo(unsigned char* buffer)
{
    unsigned char sendBuffer[1024];
    COMMAND_0X16_INFO pack;

    int totalLength = 0;
    int vLength = 0;
    int rtrn,i = 0;


    memset( sendBuffer, 0, sizeof( sendBuffer ) );
    memset( &pack, 0, sizeof( COMMAND_0X16_INFO ) );	// COMMAND_0X16_INFO data start offset 6
    memcpy( &pack, buffer+6, sizeof( COMMAND_0X16_INFO ) );

    WriteDirectInterfaceInfoConfig( DIRECTINTERFACEINFOPATH, &pack);

    vLength = sizeof(COMMAND_0X16_INFO);
    totalLength = vLength + 3;	// type 1byte, length 2byte

    sendBuffer[0] = 0xFE;
    sendBuffer[1] = totalLength >> 8;	// total length;
    sendBuffer[2] = totalLength;	// total length;
    sendBuffer[3] = SETDIRECTINTERFACEINFO;	// 0x17
    sendBuffer[4] = vLength >> 8;
    sendBuffer[5] = vLength;

    memcpy( sendBuffer+6, &pack, vLength );

    sendBuffer[totalLength+3] = 0xFF;

    rtrn = sendto( udp, sendBuffer, totalLength+4, 0,( struct sockaddr*)&client_addr, sizeof( client_addr));

    for( i = 0; i < rtrn; i++ )
	printf("%02X ", sendBuffer[i]);
    printf("\n");
   
}



static void onMessage(redisAsyncContext *c, void *reply, void *privdata) {

    int status, offset;
    int i,j,pointOffset;
    redisReply *r = reply;
    time_t  transferTime, sensingTime;
    unsigned char savePac[1024];
    unsigned char pointUpdateBuffer[1024];
    POINTINFO_VAR   point[8];

    UINT8 parsing[256][256];
    UINT8 parsingCnt = 0;

    int totalLength = 0;
    int vLength = 0;
    int rtrn = 0;

    char* ptr;
    char* port;
    char str[1024];

    UINT8 buffer[8];

    if (reply == NULL) return;

    if (r->type == REDIS_REPLY_ARRAY) {
	for (j = 0; j < r->elements; j++) {
	    printf("%u) %s\n", j, r->element[j]->str);

	}

	if (strcasecmp(r->element[0]->str,"message") == 0 &&
		strcasecmp(r->element[1]->str,"callbackClearCommand") == 0) {

	    buffer[0] = 0xFE;
	    buffer[1] = 0x00;
	    buffer[2] = 0x04;
	    buffer[3] = 0x06;
	    buffer[4] = 0x00;
	    buffer[5] = 0x01;
	    buffer[6] = 0x00;
	    buffer[7] = 0xFF;

	    Pack_Manager( buffer, 8 );
	    /*
	    memset( pointUpdateBuffer, 0, sizeof( pointUpdateBuffer) );
	    memset( point, 0, sizeof( POINTINFO_VAR) * 8);

	    vLength = sizeof( POINTINFO_VAR  ) * 8; // DI PORT is 8
	    totalLength = vLength + 3;	// type 1byte, length 2byte

	    pointUpdateBuffer[0] = 0xFE;
	    pointUpdateBuffer[1] = totalLength >> 8;	// total length;
	    pointUpdateBuffer[2] = totalLength;	// total length;
	    pointUpdateBuffer[3] = 0x13;
	    pointUpdateBuffer[4] = vLength >> 8;
	    pointUpdateBuffer[5] = vLength;



	    strcpy( str, r->element[2]->str );
	    printf("%s\n", str);

	    ptr = strtok( str, ";" ) ;
	    printf("%s\n", ptr);
	    pointOffset = 0;
	    point[pointOffset].port = 0;
	    point[pointOffset].value = atoi(ptr);
	    pointOffset++;

	    while( ptr = strtok( NULL, ";"))
	    {
		printf( "%s\n", ptr); 

		point[pointOffset].port = pointOffset;
		point[pointOffset].value = atoi(ptr);
		pointOffset++;
	    }

	    memcpy( pointUpdateBuffer+6, &point, vLength );
	    pointUpdateBuffer[totalLength+3] = 0xFF;

	    rtrn = sendto( udp, pointUpdateBuffer, totalLength+4, 0,( struct sockaddr*)&client_addr, sizeof( client_addr));
	    printf("point value%d\n", rtrn);
	    */

	}

    }
}


void *thread_main(void *arg)
{


    debugPrintf(configInfo.debug, "start Thread [SUBSCRIBE callbackClearCommand]");
    struct event_base *base = event_base_new();
    redisAsyncContext *c = redisAsyncConnect("127.0.0.1", 6379);
    if (c->err) {
	// Let *c leak for now... 
	debugPrintf(configInfo.debug, "Error: %s\n", c->errstr);
	return;
    }
    redisLibeventAttach(c, base);
    redisAsyncCommand(c, onMessage, NULL, "SUBSCRIBE callbackClearCommand");
    event_base_dispatch(base);

    debugPrintf(configInfo.debug, "End Thread [SUBSCRIBE callbackClearCommand]");
    pthread_exit((void *) 0);
}


