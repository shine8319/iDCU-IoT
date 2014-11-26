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

void WriteNetworkUseConfig(UINT8 *name, UINT8 *lan, UINT8 *wlan )
{

    FILE    *config_fp;

    config_fp = fopen( name, "w" ) ;

    fprintf( config_fp, "lan %d\n", *lan);
    fprintf( config_fp, "wlan %d\n", *wlan);

    fclose( config_fp );
}
