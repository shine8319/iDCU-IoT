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
    fprintf( config_fp, "	<PORT>%d</PORT>\n", connect->port);
    fprintf( config_fp, "	<LOCALPORT>%d</LOCALPORT>\n", connect->localport);
    fprintf( config_fp, "	<TIMEOUT>%d</TIMEOUT>\n", connect->timeout);

    fprintf( config_fp, "   </CONNECTINFO>\n" );
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


