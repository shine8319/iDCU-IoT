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
//#include "./include/UDPConnect.h"

#define DBPATH "/work/db/dabom-device"
#define SQLITE_SAFE_FREE(x)	if(x){ x = NULL; }
#define	BUFF_SIZE 1024
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

struct sockaddr_in   client_addr;
int client_addr_size;
int udp;


DEVINFO	    device;
DEVINFO	    saveDev;


sqlite3 *pSQLite3;

int client_sock; 



void writeWlan0SecurityConfig( unsigned char* buffer, int len)
{

    FILE    *fp_log;
    int i =1;

    fp_log = fopen( "/work/script/network/wlan/security/wpa_supplicant.conf", "w");
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
			strcpy( security->wep104[security->key_flag], token );
			printf("security->wep104:%s\n\n", security->wep104[security->key_flag] ) ;
		    }
		    else if( security->group == 4 )
		    {
			strcpy( security->wep40[security->key_flag], token );
			printf("security->wep40:%s\n\n", security->wep40[security->key_flag] ) ;
		    }
		    else
		    {
			printf("security->group:%d\n\n", security->group) ;

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

    fprintf( config_fp, "auth_alg=OPEN\n" );

    if( security->key_mgmt == 1 )
    {
	switch( security->group ) 
	{
	    case 3:
		fprintf( config_fp, "group=WEP104\n" );

		token = strtok( security->wep104[0], " " ) ;
		fprintf( config_fp, "wep_key0=\"%s\"\n", token );
		/*
		fprintf( config_fp, "wep_key1 %s\n", security->wep104[1] );
		fprintf( config_fp, "wep_key2 %s\n", security->wep104[2] );
		fprintf( config_fp, "wep_key3 %s\n", security->wep104[3] );
		*/
		//fprintf( config_fp, "wep_tx_keyidx %d\n", security->key_flag );
		break;

	    case 4:
		fprintf( config_fp, "group=WEP40\n" );
		token = strtok( security->wep40[0], " " ) ;
		fprintf( config_fp, "wep_key0=\"%s\"\n", token );
		/*
		fprintf( config_fp, "wep_key1 %s\n", security->wep40[1] );
		fprintf( config_fp, "wep_key2 %s\n", security->wep40[2] );
		fprintf( config_fp, "wep_key3 %s\n", security->wep40[3] );
		*/
		//fprintf( config_fp, "wep_tx_keyidx %d\n", security->key_flag );
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

    fprintf( config_fp, "auth_alg OPEN\n" );

    if( security->key_mgmt == 1 )
    {
	switch( security->group ) 
	{
	    case 3:
		fprintf( config_fp, "group WEP104\n" );

		token = strtok( security->wep104[0], " " ) ;
		fprintf( config_fp, "wep_key0 %s\n", token );
		/*
		fprintf( config_fp, "wep_key1 %s\n", security->wep104[1] );
		fprintf( config_fp, "wep_key2 %s\n", security->wep104[2] );
		fprintf( config_fp, "wep_key3 %s\n", security->wep104[3] );
		*/
		//fprintf( config_fp, "wep_tx_keyidx %d\n", security->key_flag );
		break;

	    case 4:
		fprintf( config_fp, "group WEP40\n" );
		token = strtok( security->wep40[0], " " ) ;
		fprintf( config_fp, "wep_key0 %s\n", token );
		/*
		fprintf( config_fp, "wep_key1 %s\n", security->wep40[1] );
		fprintf( config_fp, "wep_key2 %s\n", security->wep40[2] );
		fprintf( config_fp, "wep_key3 %s\n", security->wep40[3] );
		*/
		//fprintf( config_fp, "wep_tx_keyidx %d\n", security->key_flag );
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

    //Connect_Manager();
    udp = UDPConnect( 50005 );
    printf("UDPConnect %d\n", udp);
    Socket_Manager( &udp ); 

    /*******************************************************/
    el1000_sqlite3_close( &pSQLite3 );

    printf("End\n");

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

   rtrn = setsockopt(sock, SOL_SOCKET, SO_BROADCAST, &broadcastEnable, sizeof(broadcastEnable));

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

    while( 1 ) 
    {

    //printf("go\n");
    //sleep(10);
	//ReadMsgSize = recvfrom( *fd, DataBuf, BUFFER_SIZE, 0 , ( struct sockaddr*)&client_addr, &client_addr_size);
	ReadMsgSize = recvfrom( udp, DataBuf, BUFFER_SIZE, 0 , ( struct sockaddr*)&client_addr, &client_addr_size);
	if( ReadMsgSize > 0 ) 
	{

    printf("full\n");
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

    for( i = 0; i < len; i++ )
    {

	if( cvalue[i] == 0xFE )
	{

	    if( len >= i + cvalue[i+1] && ( cvalue[i + (cvalue[i+1]+2)] == 0xFF ) )
	    {
		memcpy( setBuffer, cvalue+i, cvalue[i+1]+3 );

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

	    ReadNetworkConfig("/work/config/eth0.config", &device.lan);
	    ReadNetworkConfig("/work/config/wlan0.config", &device.wlan);
	    ReadSecurityConfig("/work/config/security.config", &device.security);

	    ReadNetworkUseConfig("/work/config/networkUse.config", &device.lanEnable, &device.wlanEnable );

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
	    memcpy( &saveDev, buffer+4, sizeof( DEVINFO ) );

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


	    WriteNetworkUseConfig("/work/config/networkUse.config", &saveDev.lanEnable, &saveDev.wlanEnable );

	    if(saveDev.lanEnable )
	    {
		WriteNetworkConfig("/work/config/eth0.config", &saveDev.lan);
		rtrn = system("/work/script/network/lan/eth0.sh");
		printf("system() %d\n", rtrn);

		if( rtrn != 0 )
		    sendBuffer[7] = 0x01;

	    }

	    if( saveDev.wlanEnable )
	    {
		WriteNetworkConfig("/work/config/wlan0.config", &saveDev.wlan);
		WriteSecurityConfig("/work/config/security.config", &saveDev.security);
		WriteSecurityScript("/work/script/network/wlan/security/wpa_supplicant.conf", &saveDev.security);
		rtrn = system("/work/script/network/wlan/wlan0.sh");
		printf("system() %d\n", rtrn);

		if( rtrn != 0 )
		    sendBuffer[7] = 0x01;
	    }

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
