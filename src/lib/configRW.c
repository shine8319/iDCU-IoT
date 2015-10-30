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

#include "../include/iDCU.h"

UINT32 ReadTransducer(UINT8 *name, UINT16 *transducerId )
{
    FILE* config_fp ;
    char line[1024 + 1] ;
    char* token ;
    int count = 0;

    config_fp = fopen( name, "r+" ) ;

    while( fgets( line, 1024, config_fp ) != NULL )
    {
	token = strtok( line, "\t =\n\r" ) ;
	if( token != NULL && token[0] != '#' )
	{
	    token = strtok( NULL, "\t =\n\r" ) ;
	    transducerId[count] = atoi( token );
	    count++;
	}
    }
    fclose( config_fp );
    return count;
}


int ReadDeviceInfo(UINT8 *name, UINT8 *type, UINT8 *value )
{
    FILE* config_fp ;
    char line[1024 + 1] ;
    char* token ;
    int rtrn = 0;

    config_fp = fopen( name, "r+" ) ;

    while( fgets( line, 1024, config_fp ) != NULL )
    {
	token = strtok( line, "\t =\n\r" ) ;
	if( token != NULL && token[0] != '#' )
	{
	    
	    if( !strcmp( token, type ) ) 
	    {
		token = strtok( NULL, "\t =\n\r" ) ;

		value  = (token);
		printf("get String %s\n", value );

		rtrn = 1;

	    }


	}
    }

    fclose( config_fp );
    return rtrn;
}

void ReadEnvConfig(UINT8 *name, READENV *env )
{
    FILE* config_fp ;
    char line[1024 + 1] ;
    char* token ;

    config_fp = fopen( name, "r+" ) ;

    while( fgets( line, 1024, config_fp ) != NULL )
    {
	token = strtok( line, "\t =\n\r" ) ;
	if( token != NULL && token[0] != '#' )
	{
	    
	    if( !strcmp( token, "middleware_ip" ) ) 
	    {
		token = strtok( NULL, "\t =\n\r" ) ;

		env->middleware_ip  = inet_addr(token);
		//printf("get IP %s\n", env->middleware_ip );

	    }
	    else if( !strcmp( token, "middleware_port" ) ) 
	    {
		token = strtok( NULL, "\t =\n\r" ) ;

		env->middleware_port  = atoi(token);

	    }
	    else if( !strcmp( token, "manufacturer" ) ) 
	    {
		token = strtok( NULL, "\t =\n\r" ) ;

		sprintf(env->manufacturer, "%s",token);

	    }
	    else if( !strcmp( token, "producno" ) ) 
	    {
		token = strtok( NULL, "\t =\n\r" ) ;

		sprintf(env->producno, "%s",token);

	    }
	    else if( !strcmp( token, "registration" ) ) 
	    {
		token = strtok( NULL, "\t =\n\r" ) ;

		env->registration  = atoi( token );

	    }
	    else if( !strcmp( token, "gatenode" ) ) 
	    {
		token = strtok( NULL, "\t =\n\r" ) ;

		env->gatenode  = atoi( token );

	    }
	    else if( !strcmp( token, "pan" ) ) 
	    {
		token = strtok( NULL, "\t =\n\r" ) ;

		env->pan  = atoi( token );

	    }
	    else if( !strcmp( token, "sensornode" ) ) 
	    {
		token = strtok( NULL, "\t =\n\r" ) ;

		env->sensornode = atoi( token );

	    }
	    else if( !strcmp( token, "timeout" ) ) 
	    {
		token = strtok( NULL, "\t =\n\r" ) ;

		env->timeout = atoi( token );

	    }







	}
    }

    fclose( config_fp );
}

void ReadNetworkUseConfig(UINT8 *name, UINT8 *lan, UINT8 *wlan )
{
    FILE* config_fp ;
    char line[1024 + 1] ;
    char* token ;

    config_fp = fopen( name, "r+" ) ;

    while( fgets( line, 1024, config_fp ) != NULL )
    {
	token = strtok( line, "\t =\n\r" ) ;
	if( token != NULL && token[0] != '#' )
	{
	    
	    if( !strcmp( token, "lan" ) ) 
	    {
		token = strtok( NULL, "\t =\n\r" ) ;

		*lan  = atoi( token );
		 
		printf("lan enable = %d\n", *lan );

	    }
	    else if( !strcmp( token, "wlan" ) ) 
	    {
		token = strtok( NULL, "\t =\n\r" ) ;

		*wlan  = atoi( token );
		 
		printf("wlan enable = %d\n", *wlan );

	    }


	}
    }

    fclose( config_fp );
}
void ReadOptionsConfig(UINT8 *name, UINT8 *lan, UINT8 *wlan )
{
    FILE* config_fp ;
    char line[1024 + 1] ;
    char* token ;

    config_fp = fopen( name, "r+" ) ;

    while( fgets( line, 1024, config_fp ) != NULL )
    {
	token = strtok( line, "\t =\n\r" ) ;
	if( token != NULL && token[0] != '#' )
	{
	    
	    if( !strcmp( token, "lanauto" ) ) 
	    {
		token = strtok( NULL, "\t =\n\r" ) ;

		*lan  = atoi( token );
		 
		printf("lan auto = %d\n", *lan );

	    }
	    else if( !strcmp( token, "wlanauto" ) ) 
	    {
		token = strtok( NULL, "\t =\n\r" ) ;

		*wlan  = atoi( token );
		 
		printf("wlan auto = %d\n", *wlan );

	    }


	}
    }

    fclose( config_fp );
}


void WriteNetworkUseConfig(UINT8 *name, UINT8 *lan, UINT8 *wlan )
{

    FILE    *config_fp;

    config_fp = fopen( name, "w" ) ;

    fprintf( config_fp, "lan %d\n", *lan);
    fprintf( config_fp, "wlan %d\n", *wlan);

    fclose( config_fp );
}
void WriteOptionsConfig(UINT8 *name, UINT8 *lan, UINT8 *wlan )
{

    FILE    *config_fp;

    config_fp = fopen( name, "w" ) ;

    fprintf( config_fp, "lanauto %d\n", *lan);
    fprintf( config_fp, "wlanauto %d\n", *wlan);

    fclose( config_fp );
}

void WriteSerialConfig( UINT8 *name, COMPORTINFO_VAR  *comport, CONNECTINFO_VAR *connect )
{

    struct sockaddr_in servaddr; //server addr
    FILE    *config_fp;
    struct lan_var device;
    SAVEINFO    save;

    char    *strIP;
    int i = 0;

    //fp_log = fopen( "/work/config/eth0.config", "w");
    config_fp = fopen( name, "w" ) ;

    //memcpy( &device, buffer+4, sizeof( DEVINFO ) );

    fprintf( config_fp, "<?xml version=\"1.0\" encoding=\"UTF-8\"?>\n" );
    fprintf( config_fp, "\n" );
    fprintf( config_fp, "<SINK ver=\"1.0\">\n" );
    fprintf( config_fp, "\n" );
    fprintf( config_fp, "   <COMPORTINFO>\n" );
    switch(comport->baud) {
	case 0:
	    fprintf( config_fp, "	<BAUD>1200</BAUD>\n" );
	    break;
	case 1:
	    fprintf( config_fp, "	<BAUD>2400</BAUD>\n" );
	    break;
	case 2:
	    fprintf( config_fp, "	<BAUD>4800</BAUD>\n" );
	    break;
	case 3:
	    fprintf( config_fp, "	<BAUD>9600</BAUD>\n" );
	    break;
	case 4:
	    fprintf( config_fp, "	<BAUD>19200</BAUD>\n" );
	    break;
	case 5:
	    fprintf( config_fp, "	<BAUD>38400</BAUD>\n" );
	    break;
	case 6:
	    fprintf( config_fp, "	<BAUD>57600</BAUD>\n" );
	    break;
	case 7:
	    fprintf( config_fp, "	<BAUD>115200</BAUD>\n" );
	    break;
	case 8:
	    fprintf( config_fp, "	<BAUD>230400</BAUD>\n" );
	    break;

    }

    switch( comport->parity ) {
	case 0:
	    fprintf( config_fp, "	<PARITY>N</PARITY>\n" );
	    break;
	case 1:
	    fprintf( config_fp, "	<PARITY>E</PARITY>\n" );
	    break;
	case 2:
	    fprintf( config_fp, "	<PARITY>O</PARITY>\n" );
	    break;
    }
 
    switch( comport->databit ) {
	case 0:
	    fprintf( config_fp, "	<DATABIT>5</DATABIT>\n" );
	    break;
	case 1:
	    fprintf( config_fp, "	<DATABIT>6</DATABIT>\n" );
	    break;
	case 2:
	    fprintf( config_fp, "	<DATABIT>7</DATABIT>\n" );
	    break;
	case 3:
	    fprintf( config_fp, "	<DATABIT>8</DATABIT>\n" );
	    break;
    }


    switch( comport->stopbit ) {
	case 0:
	    fprintf( config_fp, "	<STOPBIT>1</STOPBIT>\n" );
	    break;
	case 1:
	    fprintf( config_fp, "	<STOPBIT>2</STOPBIT>\n" );
	    break;
    }

    switch( comport->flowcontrol ) {
	case 0:
	    fprintf( config_fp, "	<FLOWCONTROL>N</FLOWCONTROL>\n" );
	    break;
	case 1:
	    fprintf( config_fp, "	<FLOWCONTROL>S</FLOWCONTROL>\n" );
	    break;
	case 2:
	    fprintf( config_fp, "	<FLOWCONTROL>H</FLOWCONTROL>\n" );
	    break;
    }
 
    fprintf( config_fp, "   </COMPORTINFO>\n" );
    fprintf( config_fp, "\n" );


    fprintf( config_fp, "   <CONNECTINFO>\n" );
    switch( connect->mode) { 
	case 0:
	    fprintf( config_fp, "	<MODE>SERVER</MODE>\n" );
	    break;
	case 1:
	    fprintf( config_fp, "	<MODE>CLIENT</MODE>\n" );
	    break;
    }

    servaddr.sin_addr.s_addr = htonl(connect->ip);
    strIP = inet_ntoa(servaddr.sin_addr);
    printf("%s\n", strIP);
    fprintf( config_fp, "	<IP>%s</IP>\n", strIP );
    fprintf( config_fp, "	<PORT>%ld</PORT>\n", connect->port);
    fprintf( config_fp, "	<LOCALPORT>%ld</LOCALPORT>\n", connect->localport);
    fprintf( config_fp, "	<TIMEOUT>%ld</TIMEOUT>\n", connect->timeout);

    fprintf( config_fp, "   </CONNECTINFO>\n" );
    fprintf( config_fp, "\n" );
    fprintf( config_fp, "</SINK>\n" );


    fclose( config_fp );
}

void WriteConfigConfig( UINT8 *name, CONFIGINFO_VAR  *config)
{

    struct sockaddr_in servaddr; //server addr
    FILE    *config_fp;
    struct lan_var device;
    SAVEINFO    save;

    char    *strIP;
    int i = 0;

    config_fp = fopen( name, "w" ) ;


    fprintf( config_fp, "<?xml version=\"1.0\" encoding=\"UTF-8\"?>\n" );
    fprintf( config_fp, "\n" );
    fprintf( config_fp, "<SINK ver=\"1.0\">\n" );
    fprintf( config_fp, "\n" );

    fprintf( config_fp, "   <CONFIGURATION>\n" );
    fprintf( config_fp, "	<DEBUG>1</DEBUG>\n" );
    //fprintf( config_fp, "	<SETUPPORT>%ld</SETUPPORT>\n", config->setupport);
    fprintf( config_fp, "	<SETUPPORT>50005</SETUPPORT>\n");
    fprintf( config_fp, "   </CONFIGURATION>\n" );

    fprintf( config_fp, "   <DIGITAL>\n" );
    fprintf( config_fp, "	<PORT1INTERVAL>%d</PORT1INTERVAL>\n", config->interval[0] );
    fprintf( config_fp, "	<PORT2INTERVAL>%d</PORT2INTERVAL>\n", config->interval[1] );
    fprintf( config_fp, "	<PORT3INTERVAL>%d</PORT3INTERVAL>\n", config->interval[2] );
    fprintf( config_fp, "	<PORT4INTERVAL>%d</PORT4INTERVAL>\n", config->interval[3] );
    fprintf( config_fp, "	<PORT5INTERVAL>%d</PORT5INTERVAL>\n", config->interval[4] );
    fprintf( config_fp, "	<PORT6INTERVAL>%d</PORT6INTERVAL>\n", config->interval[5] );
    fprintf( config_fp, "	<PORT7INTERVAL>%d</PORT7INTERVAL>\n", config->interval[6] );
    fprintf( config_fp, "	<PORT8INTERVAL>%d</PORT8INTERVAL>\n", config->interval[7] );
    fprintf( config_fp, "   </DIGITAL>\n" );
    fprintf( config_fp, "\n" );
    fprintf( config_fp, "</SINK>\n" );


    fclose( config_fp );
}


void WriteTimeSyncConfig( UINT8 *name, TIMESYNCINFO_VAR  *timeSync )
{

    struct sockaddr_in servaddr; //server addr
    FILE    *config_fp;
    struct lan_var device;
    SAVEINFO    save;

    char    *strIP;
    int i = 0;

    //fp_log = fopen( "/work/config/eth0.config", "w");
    config_fp = fopen( name, "w" ) ;

    //memcpy( &device, buffer+4, sizeof( DEVINFO ) );

    fprintf( config_fp, "<?xml version=\"1.0\" encoding=\"UTF-8\"?>\n" );
    fprintf( config_fp, "\n" );
    fprintf( config_fp, "<SINK ver=\"1.0\">\n" );
    fprintf( config_fp, "\n" );

    fprintf( config_fp, "   <TIMESYNCINFO>\n" );

    servaddr.sin_addr.s_addr = htonl(timeSync->address);
    strIP = inet_ntoa(servaddr.sin_addr);
    printf("%s\n", strIP);
    fprintf( config_fp, "	<ADDRESS>%s</ADDRESS>\n", strIP );
    switch( timeSync->cycle ) { 
	case 0:
	    fprintf( config_fp, "	<CYCLE>runMinute</CYCLE>\n" );
	    break;
	case 1:
	    fprintf( config_fp, "	<CYCLE>run30Minute</CYCLE>\n" );
	    break;
	case 2:
	    fprintf( config_fp, "	<CYCLE>runHour</CYCLE>\n" );
	    break;
	case 3:
	    fprintf( config_fp, "	<CYCLE>run6Hour</CYCLE>\n" );
	    break;
	case 4:
	    fprintf( config_fp, "	<CYCLE>run12Hour</CYCLE>\n" );
	    break;
	case 5:
	    fprintf( config_fp, "	<CYCLE>runDay</CYCLE>\n" );
	    break;
    }

    fprintf( config_fp, "   </TIMESYNCINFO>\n" );
    fprintf( config_fp, "\n" );
    fprintf( config_fp, "</SINK>\n" );


    fclose( config_fp );
}





int check_interval(UINT8 value )
{
    int interval = 5;

    switch( value )
    {
	case 0:
	    interval = 5;   // 50 mSec
	    break;
	case 1:
	    interval = 10;  // 100 mSec
	    break;
	case 2:
	    interval = 15;
	    break;
	case 3:
	    interval = 20;
	    break;
	case 4:
	    interval = 25;
	    break;
	case 5:
	    interval = 30;
	    break;
	case 6:
	    interval = 50;
	    break;
	case 7:
	    interval = 100;
	    break;

    }

    return interval;
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


