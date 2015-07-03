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

#define DBPATH "/work/smart/db/comm"
#define SQLITE_SAFE_FREE(x)	if(x){ x = NULL; }
#define	BUFF_SIZE 1024
unsigned char main_quit = 0;
unsigned char user_quit = 0;


static CONFIGINFO configInfo;



extern int el1000_sqlite3_open( char *path, sqlite3 **pSQLite3 );
extern int el1000_sqlite3_customize( sqlite3 **pSQLite3 );
extern int el1000_sqlite3_close( sqlite3 **pSQLite3 );
extern int el1000_sqlite3_transaction( sqlite3 **pSQLite3, char status );
extern int el1000_sqlite3_update( sqlite3 **pSQLite3, char *query );
extern Return_Selectdata el1000_sqlite3_select( sqlite3 *pSQLite3, char *query );
extern int M2MProtocol( void* socket_fd, sqlite3 *pSQLite3, QUERYTIME time, char type, int keepdata);
extern Return_Selectdata getNodeInfo( sqlite3 *pSQLite3, char *query );
extern int el1000_sqlite3_delete( sqlite3 *pSQLite3, char *query );

int Socket_Manager( int *fd);
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

void ReadNetworkConfig(UINT8 *name, struct lan_var *lan);
void ReadSecurityConfig(UINT8 *name, struct security_var *security);
int UDPConnect( int port );
static int getPointInfo( void );
void *thread_main(void *);

typedef struct _PAC {
    char type;
    char length;
    char value[4];

} _PAC;


static redisContext *c;

int m2mid;
int eventid;

AustemStatusSendData austemStatusSend[256];
AustemSendData austemSend[2048];
unsigned char tlvSendBuffer[20480];

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



void writeWlan0SecurityConfig( unsigned char* buffer, int len)
{

    FILE    *fp_log;
    int i =1;

    fp_log = fopen( "/work/smart/script/network/wlan/security/wpa_supplicant.conf", "w");
    //printf("pf_log %d\n", fp_log);

    fprintf( fp_log, "network={\n" );
    fprintf( fp_log, "ssid=\"iDCU-AP\"\n" );
    fprintf( fp_log, "key_mgmt=NONE\n" );
    fprintf( fp_log, "wep_key0=\"acs1234567890\"\n" );
    fprintf( fp_log, "}" );

    fclose( fp_log );
}

int check_group(UINT8 *value )
{

    if( !strcmp( "CCMP", value ) )
    {
	return 1;
    }
    else if ( !strcmp( "TKIP", value ) )
    {
	return 2;
    }
    else if ( !strcmp( "WEP104", value ) )
    {
	return 3;
    }
    else if ( !strcmp( "WEP40", value ) )
    {
	return 4;
    }

    return 0;
}


int check_auth_alg(UINT8 *value )
{

    if( !strcmp( "OPEN", value ) )
    {
	return 0;
    }
    else if ( !strcmp( "SHARED", value ) )
    {
	return 1;
    }
    else if ( !strcmp( "LEAP", value ) )
    {
	return 2;
    }

    return 0;
}


int check_key_mgmt(UINT8 *value )
{

    if( !strcmp( "WPA-PSK", value ) )
    {
	return 1;
    }
    else if ( !strcmp( "WPA-EAP", value ) )
    {
	return 2;
    }
    else if ( !strcmp( "IEEE8021X", value ) )
    {
	return 3;
    }
    else if ( !strcmp( "NONE", value ) )
    {
	return 0;
    }

    return 0;
}

void ReadSecurityConfig(UINT8 *name, struct security_var *security)
{
    FILE* config_fp ;
    char line[1024 + 1] ;
    char* token ;
    UINT8 rtrn;
    char wep_key[16] = "wep_key1";
    char wep40Cnt = 0;
    char wep104Cnt = 0;

    //sprintf( wep_key, "wep_key%d\n", security->key_flag );

    config_fp = fopen( name, "r+" ) ;

    while( fgets( line, 1024, config_fp ) != NULL )
    {
	token = strtok( line, "\t =\n\r" ) ;

	//printf("else~~~ token:%s wep_key:%s\n", token, wep_key);

	if( token != NULL && token[0] != '#' )
	{

	    if( !strcmp( token, "ssid" ) ) 
	    {
		token = strtok( NULL, "\t =\n\r" ) ;

		strcpy( security->ssid, token );
		printf("security->ssid:%s\n\n", security->ssid ) ;

	    }
	    else if( !strcmp( token, "key_mgmt" ) ) 
	    {
		token = strtok( NULL, "\t =\n\r" ) ;

		security->key_mgmt = check_key_mgmt( token );
		if( security->key_mgmt == 1 )
		    security->group = 0;
		// 1 is wpa-psk
		// 0 is wep

		printf("security->key_mgmt:%d\n\n", security->key_mgmt ) ;

	    }
	    else if( !strcmp( token, "auth_alg" ) ) 
	    {
		token = strtok( NULL, "\t =\n\r" ) ;

		security->auth_alg = check_auth_alg( token );

		printf("security->auth_alg:%d\n\n", security->auth_alg) ;

	    }
	    else if( !strcmp( token, "wep_tx_keyidx" ) ) 
	    {
		token = strtok( NULL, "\t =\n\r" ) ;

		security->key_flag = atoi( token );
		//sprintf(wep_key, "wep_key%d", security->key_flag);

		//printf("%s\n", wep_key);

		printf("security->key_flag:%d\n\n", security->key_flag) ;

	    }
	    else if( !strcmp( token, "group" ) ) 
	    {
		token = strtok( NULL, "\t =\n\r" ) ;

		security->group = check_group( token );

		printf("security->group:%d\n\n", security->group) ;

	    }
	    else
	    {
		token = strtok( NULL, "\t =\n\r" ) ;
		if( token != NULL )
		{

		    if( security->group == 3 )
		    {
			strcpy( security->wep104[wep104Cnt], token );
			printf("security->wep104[%d]:%s\n\n", wep104Cnt, security->wep104[wep104Cnt] ) ;
			wep104Cnt++;
		    }
		    else if( security->group == 4 )
		    {
			strcpy( security->wep40[wep40Cnt], token );
			printf("88 security->wep40[%d]:%s\n\n",wep40Cnt, security->wep40[wep40Cnt] ) ;
			wep40Cnt++;
		    }
		    else
		    {
			strcpy( security->psk, token );
			printf("security->psk:%s\n\n", security->psk ) ;

		    }
		}
	    }


	}
    }

    fclose( config_fp );
}

void ReadNetworkConfig(UINT8 *name, struct lan_var *lan)
{
    FILE* config_fp ;
    char line[1024 + 1] ;
    char* token ;
    char ip[32];
    char convertip[32];
    unsigned long test_addr = 0;
    struct sockaddr_in servaddr; //server addr
    UINT8 rtrn;

    memset( ip, 0, sizeof( 32 ) );
    memset( convertip, 0, sizeof( 32 ) );

    config_fp = fopen( name, "r+" ) ;

    while( fgets( line, 1024, config_fp ) != NULL )
    {
	token = strtok( line, "\t =\n\r" ) ;
	if( token != NULL && token[0] != '#' )
	{

	    if( !strcmp( token, "address" ) ) 
	    {
		token = strtok( NULL, "\t =\n\r" ) ;

		lan->lan_addr = inet_addr( token );

		printf("inet_addr = %ld\n", lan->lan_addr );
		printf("value:%s\n\n", token ) ;

	    }
	    else if (!strcmp( token, "netmask" ) )
	    {
		token = strtok( NULL, "\t =\n\r" ) ;

		lan->lan_netmask = inet_addr( token );

		printf("inet_addr = %ld\n", lan->lan_netmask );
		printf("value:%s\n\n", token ) ;

	    }
	    else if (!strcmp( token, "gateway" ) )
	    {
		token = strtok( NULL, "\t =\n\r" ) ;

		lan->lan_gateway = inet_addr( token );

		printf("inet_addr = %ld\n", lan->lan_gateway );
		printf("value:%s\n\n", token ) ;

	    }
	    else if (!strcmp( token, "dns" ) )
	    {
		token = strtok( NULL, "\t =\n\r" ) ;

		lan->lan_dns = inet_addr( token );

		printf("inet_addr = %ld\n", lan->lan_dns );
		printf("value:%s\n\n", token ) ;

	    }


	}
    }

    fclose( config_fp );
}
void WriteSecurityScript(UINT8 *name, struct security_var *security)
{

    FILE    *config_fp;

    char* token ;

    config_fp = fopen( name, "w" ) ;

    fprintf( config_fp, "network={\n" );

    token = strtok( security->ssid, " " ) ;
    fprintf( config_fp, "ssid=\"%s\"\n", token );

    switch( security->key_mgmt )
    {
	case 0:
	    fprintf( config_fp, "key_mgmt=NONE\n" );
	    break;

	case 1:
	    fprintf( config_fp, "key_mgmt=NONE\n" );
	    break;

	case 2:
	    fprintf( config_fp, "key_mgmt=WPA-PSK\n" );
	    break;

	    /*
	       fprintf( config_fp, "key_mgmt WPA-EAP\n" );
	       case 3:
	       fprintf( config_fp, "key_mgmt IEEE8021X\n" );
	       break;
	     */
    }

    //fprintf( config_fp, "auth_alg=OPEN\n" );

    if( security->key_mgmt == 1 )
    {
	switch( security->group ) 
	{
	    case 3:
		fprintf( config_fp, "group=WEP104\n" );

		fprintf( config_fp, "wep_key0=\"%.13s\"\n", security->wep104[0] );
		fprintf( config_fp, "wep_key1=\"%.13s\"\n", security->wep104[1] );
		fprintf( config_fp, "wep_key2=\"%.13s\"\n", security->wep104[2] );
		fprintf( config_fp, "wep_key3=\"%.13s\"\n", security->wep104[3] );

		fprintf( config_fp, "wep_tx_keyidx=%d\n", security->key_flag );
		break;

	    case 4:
		fprintf( config_fp, "group=WEP40\n" );
		fprintf( config_fp, "wep_key0=\"%.5s\"\n", security->wep40[0] );
		fprintf( config_fp, "wep_key1=\"%.5s\"\n", security->wep40[1] );
		fprintf( config_fp, "wep_key2=\"%.5s\"\n", security->wep40[2] );
		fprintf( config_fp, "wep_key3=\"%.5s\"\n", security->wep40[3] );

		fprintf( config_fp, "wep_tx_keyidx=%d\n", security->key_flag );
		break;

	}
    }
    else if( security->key_mgmt == 2 )
    {
	token = strtok( security->psk, " " ) ;
	fprintf( config_fp, "psk=\"%s\"\n", token );
    }

    fprintf( config_fp, "}" );


    fclose( config_fp );
}

void WriteSecurityConfig(UINT8 *name, struct security_var *security)
{

    FILE    *config_fp;

    char* token ;

    config_fp = fopen( name, "w" ) ;


    token = strtok( security->ssid, " " ) ;
    fprintf( config_fp, "ssid %s\n", token );

    switch( security->key_mgmt )
    {
	case 0:
	    fprintf( config_fp, "key_mgmt NONE\n" );
	    break;

	case 1:
	    fprintf( config_fp, "key_mgmt NONE\n" );
	    break;

	case 2:
	    fprintf( config_fp, "key_mgmt WPA-PSK\n" );
	    break;

	    /*
	       fprintf( config_fp, "key_mgmt WPA-EAP\n" );
	       case 3:
	       fprintf( config_fp, "key_mgmt IEEE8021X\n" );
	       break;
	     */
    }

    //fprintf( config_fp, "auth_alg OPEN\n" );

    //printf("wep_tx_keyidx %d\n", security->key_flag);
    if( security->key_mgmt == 1 )
    {
	switch( security->group ) 
	{
	    case 3:
		fprintf( config_fp, "group WEP104\n" );

		fprintf( config_fp, "wep_key0 %.13s\n", security->wep104[0] );
		fprintf( config_fp, "wep_key1 %.13s\n", security->wep104[1] );
		fprintf( config_fp, "wep_key2 %.13s\n", security->wep104[2] );
		fprintf( config_fp, "wep_key3 %.13s\n", security->wep104[3] );

		fprintf( config_fp, "wep_tx_keyidx %d\n", security->key_flag );
		break;

	    case 4:
		fprintf( config_fp, "group WEP40\n" );
		fprintf( config_fp, "wep_key0 %.5s\n", security->wep40[0] );
		fprintf( config_fp, "wep_key1 %.5s\n", security->wep40[1] );
		fprintf( config_fp, "wep_key2 %.5s\n", security->wep40[2] );
		fprintf( config_fp, "wep_key3 %.5s\n", security->wep40[3] );

		fprintf( config_fp, "wep_tx_keyidx %d\n", security->key_flag );
		break;


	}
    }
    else if( security->key_mgmt == 2 )
    {
	token = strtok( security->psk, " " ) ;
	fprintf( config_fp, "psk %s\n", token );
    }


    fclose( config_fp );
}

void WriteNetworkConfig(UINT8 *name, struct lan_var *lan)
{

    struct sockaddr_in servaddr; //server addr
    FILE    *config_fp;
    struct lan_var device;
    SAVEINFO    save[2];
    int i = 0;

    //fp_log = fopen( "/work/config/eth0.config", "w");
    config_fp = fopen( name, "w" ) ;

    //memcpy( &device, buffer+4, sizeof( DEVINFO ) );


    servaddr.sin_addr.s_addr = htonl(lan->lan_addr);
    save[i].lan_addr = inet_ntoa(servaddr.sin_addr);
    printf("%s\n", save[i].lan_addr);
    fprintf( config_fp, "address %s\n", save[i].lan_addr );

    servaddr.sin_addr.s_addr = htonl(lan->lan_netmask);
    save[i].lan_netmask = inet_ntoa(servaddr.sin_addr);
    printf("%s\n", save[i].lan_netmask);
    fprintf( config_fp, "netmask %s\n", save[i].lan_netmask );

    servaddr.sin_addr.s_addr = htonl(lan->lan_gateway);
    save[i].lan_gateway = inet_ntoa(servaddr.sin_addr);
    printf("%s\n", save[i].lan_gateway);
    fprintf( config_fp, "gateway %s\n", save[i].lan_gateway );

    servaddr.sin_addr.s_addr = htonl(lan->lan_dns);
    save[i].lan_dns = inet_ntoa(servaddr.sin_addr);
    printf("%s\n", save[i].lan_dns);
    fprintf( config_fp, "dns %s\n", save[i].lan_dns );

    fclose( config_fp );
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

    configInfo = configInfoParser("/work/smart/config/config.xml");
    c = redisInitialize();
    /******************** DB connect ***********************/
    //char	*szErrMsg;
    // DB Open
    rc = el1000_sqlite3_open( DBPATH, &pSQLite3 );
    if( rc != 0 )
    {
	writeLog( "/work/setup/log", "error DB Open" );
	return -1;
    }
    else
    {
	writeLog( "/work/setup/log", "DB Open" );
	printf("%s OPEN!!\n", DBPATH);
    }


    //sqlite3_busy_handler( pSQLite3, busy, NULL);
    sqlite3_busy_timeout( pSQLite3, 1000);
    //sqlite3_busy_timeout( pSQLite3, 5000);

    // DB Customize
    rc = el1000_sqlite3_customize( &pSQLite3 );
    if( rc != 0 )
    {
	writeLog( "/work/setup/log", "error DB Customize" );
	return -1;
    }

    getPointInfo();

    /***************** Server Connect **********************/
    if( -1 == ( m2mid = msgget( (key_t)2222, IPC_CREAT | 0666)))
    {
	writeLog( "/work/setup/log", "error msgget() m2mid" );
	//perror( "msgget() 실패");
	return -1;
    }

    if( -1 == ( eventid = msgget( (key_t)3333, IPC_CREAT | 0666)))
    {
	writeLog( "/work/setup/log", "error msgget() eventid" );
	//perror( "msgget() 실패");
	return -1;
    }

    configInfo = configInfoParser("/work/smart/config/config.xml");
    if( pthread_create(&p_thread, NULL, &thread_main, NULL) == -1 )
    {
	writeLog("/work/setup/log", "create thread_main error" );
	return -1;
    }


    //Connect_Manager();
    udp = UDPConnect( atoi(configInfo.setupport) );
    printf("UDPConnect %d\n", udp);
    Socket_Manager( &udp ); 

    /*******************************************************/
    el1000_sqlite3_close( &pSQLite3 );

    printf("End\n");
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


    rtrn = setsockopt(sock, SOL_SOCKET, SO_BROADCAST, &broadcastEnable, sizeof(broadcastEnable));
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

    printf("Receive Ready!!!\n");

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
	    rtrn = sendto( udp, tlvSendBuffer, 17, 0,( struct sockaddr*)&client_addr, sizeof( client_addr));
	    //printf("Send rtrn = %d\n", rtrn);
	    memset( tlvSendBuffer, 0, 20480 ); 
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

	    ReadNetworkConfig("/work/smart/config/eth0.config", &device.lan);
	    ReadNetworkConfig("/work/smart/config/wlan0.config", &device.wlan);
	    ReadSecurityConfig("/work/smart/config/security.config", &device.security);

	    ReadNetworkUseConfig("/work/smart/config/networkUse.config", &device.lanEnable, &device.wlanEnable );

	    ReadOptionsConfig("/work/smart/config/options.config", &device.lanAuto, &device.wlanAuto);

	    // serial tab 2015.05.15
	    xmlinfo = serialInfoParser("/work/smart/config/serial_info.xml");
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

	    if( device.lanEnable && device.lanAuto )
	    {
		memset( convertAddr, 0, 14);

		s_getIpAddress("eth0", addr);
		printf("ip addr:=%d.%d.%d.%d\n", (int)addr[0], (int)addr[1], (int)addr[2], (int)addr[3]);
		sprintf( convertAddr, "%d.%d.%d.%d", (int)addr[0], (int)addr[1], (int)addr[2], (int)addr[3]);
		device.lan.lan_addr = inet_addr( convertAddr );
		printf("inet_addr = %ld\n", device.lan.lan_addr );
	    }
	    if( device.wlanEnable && device.wlanAuto )
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

	    // timeSync tab 2015.06.04
	    timeSyncInfo = timeSyncInfoParser("/work/smart/config/timeSync.xml");
	    device.timeSync.cycle = getTimeSyncCycle(timeSyncInfo.cycle);
	    device.timeSync.address = inet_addr( timeSyncInfo.address );

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


	    WriteNetworkUseConfig("/work/smart/config/networkUse.config", &saveDev.lanEnable, &saveDev.wlanEnable );
	    WriteOptionsConfig("/work/smart/config/options.config", &saveDev.lanAuto, &saveDev.wlanAuto);

	    if(saveDev.lanEnable )
	    {
		WriteNetworkConfig("/work/smart/config/eth0.config", &saveDev.lan);
		if( !saveDev.lanAuto )
		{
		    rtrn = system("/work/smart/script/network/lan/eth0.sh &");
		    printf("system() %d\n", rtrn);

		    //if( rtrn != 0 )
		    //sendBuffer[7] = 0x01;
		}
		else
		    rtrn = system("/work/smart/script/network/lan/eth0Static.sh &");

	    }

	    if( saveDev.wlanEnable )
	    {
		WriteNetworkConfig("/work/smart/config/wlan0.config", &saveDev.wlan);
		WriteSecurityConfig("/work/smart/config/security.config", &saveDev.security);
		WriteSecurityScript("/work/smart/script/network/wlan/security/wpa_supplicant.conf", &saveDev.security);

		if( !saveDev.wlanAuto )
		{
		    rtrn = system("/work/smart/script/network/wlan/wlan0.sh &");
		    printf("system() %d\n", rtrn);

		    //if( rtrn != 0 )
		    //sendBuffer[7] = 0x01;
		}
		else
		    rtrn = system("/work/smart/script/network/wlan/wlan0Static.sh &");

	    }

	    WriteSerialConfig( "/work/smart/config/serial_info.xml", &saveDev.comport, &saveDev.connect );
	    WriteTimeSyncConfig( "/work/smart/config/timeSync.xml", &saveDev.timeSync);

	    rtrn = system("/work/smart/script/restartProgram.sh &");

	    sendBuffer[totalLength+3] = 0xFF;


	    rtrn = sendto( udp, sendBuffer, totalLength+4, 0,( struct sockaddr*)&client_addr, sizeof( client_addr));
	    printf("SET INFO %d\n", rtrn);


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
	perror( "msgsnd() commandM2MConfig");
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
	perror( "msgsnd() commandNodeConfig");
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
	//int rtrn = write(client_sock, sendBuffer, totalLength+4);
	int rtrn = sendto( udp, sendBuffer, totalLength+4, 0,( struct sockaddr*)&client_addr, sizeof( client_addr));

	for( i = 0; i < rtrn; i++ )
	    printf("%02X ", sendBuffer[i]);
	printf("\n");
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
	//int rtrn = write(client_sock, tlvSendBuffer, totalLength+4);
	int rtrn = sendto( udp, tlvSendBuffer, totalLength+4, 0,( struct sockaddr*)&client_addr, sizeof( client_addr));
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
    //int rtrn = write(client_sock, tlvSendBuffer, totalLength+4);
    int rtrn = sendto( udp, tlvSendBuffer, totalLength+4, 0,( struct sockaddr*)&client_addr, sizeof( client_addr));
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


