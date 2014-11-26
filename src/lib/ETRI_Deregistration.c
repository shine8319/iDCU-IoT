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
#include "../include/ETRI.h"
#include "../include/configRW.h"
#include "../include/TCPSocket.h"
#include "../include/sqlite3.h"
#include "../include/SQLite3Interface.h"

int ETRI_Deregistration( int *tcp, READENV *env ) {

    READENV info;

    sqlite3 *pSQLite3;
    UINT32 rc;

    memcpy( &info.manufacturer, &env->manufacturer, 20 );
    memcpy( &info.producno, &env->producno, 20 );
    info.gatenode = env->gatenode;
    info.pan = env->pan;
    info.sensornode = env->sensornode;

    rc = IoT_sqlite3_open( "/work/smart/db/driver", &pSQLite3 );
    printf("rc %ld\n", rc );
    if( rc != 0 )
    {
	writeLog( "/work/smart/log", "error DB Open" );
	return -1;
    }
    else
    {
	writeLog( "/work/smart/log", "DB Open" );
	printf("DB OPEN!!\n");
    }

    /*
    Transducer_Description_Deregistration( tcp, &info, pSQLite3 );
    SensorNode_Description_Deregistration( tcp, &info );
    PAN_Description_Deregistration( tcp, &info  );
    GateNode_Description_Deregistration( tcp, &info );
    */
    if( -1 == Transducer_Description_Deregistration( tcp, &info, pSQLite3 ) )
	return -1;

    if( -1 == SensorNode_Description_Deregistration( tcp, &info ) )
	return -1;

    if( -1 == PAN_Description_Deregistration( tcp, &info  ) )
	return -1;

    if( -1 == GateNode_Description_Deregistration( tcp, &info ) )
	return -1;

    IoT_sqlite3_close( &pSQLite3 );

    return 0 ;
}

int GateNode_Description_Deregistration( int *tcp, READENV *info )
{
    int rtrn = 0;
    Struct_GateNode env;

    env.Message_Id	    = 0x0002;
    env.Length		    = 0x0002;
    env.GateNode_Id	    = info->gatenode;

    rtrn = sender( tcp, &env, sizeof( Struct_GateNode ) );

    return rtrn;
}

int PAN_Description_Deregistration( int *tcp, READENV *info )
{
    int rtrn = 0;
    Struct_PAN env;

    env.Message_Id	    = 0x0004;
    env.Length	    = 0x0004;
    env.GateNode_Id	    = info->gatenode;
    env.PAN_Id	    = info->pan;
    
    rtrn = sender( tcp, &env, sizeof( Struct_PAN ) );

    return rtrn;
}

int SensorNode_Description_Deregistration( int *tcp, READENV *info ) 
{
    int rtrn = 0;
    Struct_SensorNode env;

    env.Message_Id	    = 0x0006;
    env.Length	    = 0x0006;
    env.GateNode_Id	    = info->gatenode;
    env.PAN_Id	    = info->pan;
    env.SensorNode_Id   = info->sensornode;

    rtrn = sender( tcp, &env, sizeof( Struct_SensorNode ) );

    return rtrn;
}

int Transducer_Description_Deregistration( int *tcp, READENV *info, sqlite3 *pSQLite3 )
{
    int count;
    UINT16 rtrn;

    SQLite3Data data;
    char *query;

    int selectOffset = 1;
    int i;
    Struct_Transducer env[64];


    memset( env, 0, sizeof( Struct_Transducer ) * 64 ); 

    query = sqlite3_mprintf("select transducerid from transducerinfo");
    data = IoT_sqlite3_select( pSQLite3, query );
    printf(" select size = %ld\n", data.size );
    if( data.size > 0 )
    {
	count = data.size - 1;

	for( i = 0; i < count; i++ )
	{
	    env[i].Message_Id	    = 0x0008;
	    env[i].Length		    = 0x0008;
	    env[i].GateNode_Id	    = info->gatenode;
	    env[i].PAN_Id		    = info->pan;
	    env[i].SensorNode_Id	    = info->sensornode;
	    env[i].Transducer_Id	    = atoi(data.data[selectOffset++]);
	    printf(" env[i].Transducer_Id %d\n", env[i].Transducer_Id );

	}
    }
    else
	count = 0;

    sqlite3_free( query );
    SQLITE_SAFE_FREE( query )
    sqlite3_free_table( data.data );
    SQLITE_SAFE_FREE( data.data )

    while( count ) 
    {

	//printf(" transducer count %d, packet length %d, test size %d\n", count, env[i].Transducer.Length, size );
	rtrn = sender( tcp, &env[count-1], sizeof( Struct_Transducer ) );
	if( rtrn == 0 )
	{
	    count--;
	}
	else
	{
	    rtrn = -1;
	    break;
	}
    }

    return rtrn;
}


int Transducer_Deregistration( int *tcp, UINT16 *transducerId )
{
    UINT8 rtrn;
    UINT8 sendBuffer[128];

    printf("1\n");
    //memcpy( sendBuffer, transducerId, sizeof( UINT16 ) );
    /*
       sendBuffer[0] = (UINT8)(transducerId  >> 0);
       sendBuffer[1] = (UINT8)(transducerId  >> 8);
     */

    printf("2\n");
    rtrn = send(*tcp, transducerId, sizeof( UINT16 ), MSG_NOSIGNAL);
    printf("send length = %d\n", rtrn);
    if( rtrn == -1 )
    {
	return -1;
    }

    return 0;
}


