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
#include <netinet/in.h>

#include "../include/iDCU.h"
#include "../include/ETRI.h"
#include "../include/configRW.h"
#include "../include/sqlite3.h"
#include "../include/SQLite3Interface.h"
#define BUFFER_SIZE 1024

int ETRI_Registration( int *tcp, READENV *env ) {

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
	writeLog( "/work/smart/log", "[ETRI_Registration] fail DB Opne" );
	return -1;
    }
    else
    {
	//writeLog( "/work/smart/log", "[ETRI_Registration] DB Open" );
	printf("DB OPEN!!\n");
    }


    if( -1 == GateNode_Description_Registration( tcp, &info ) )
	return -1;
    if( -1 == PAN_Description_Registration( tcp, &info  ) )
	return -1;
    if( -1 == SensorNode_Description_Registration( tcp, &info ) )
	return -1;
    if( -1 == Transducer_Description_Registration( tcp, &info, pSQLite3 ) )
	return -1;

    IoT_sqlite3_close( &pSQLite3 );

    return 0 ;
}

int GateNode_Description_Registration( int *tcp, READENV *info )
{
    int rtrn = 0;
    Struct_GateNode_Registration env;

    env.GateNode.Message_Id	    = 0x0001;
    env.GateNode.Length	    = 0x0036;
    env.GateNode.GateNode_Id	    = info->gatenode;
    memcpy( &env.Manufacturer, info->manufacturer, 20 );
    memcpy( &env.ProducNo, info->producno, 20 );
    env.Longitude		    = 0;
    env.Latitude		    = 0;
    env.Altitude		    = 0;

    rtrn = sender( tcp, (UINT8*)&env, sizeof( Struct_GateNode_Registration ) );

    return rtrn;
}

int PAN_Description_Registration( int *tcp, READENV *info )
{
    int rtrn = 0;
    Struct_PAN_Registration env;

    env.PAN.Message_Id	    = 0x0003;
    env.PAN.Length	    = 0x002F;
    env.PAN.GateNode_Id	    = info->gatenode;
    env.PAN.PAN_Id	    = info->pan;
    env.PAN_Channel	    = 0xFF;
    memcpy( &env.Manufacturer, info->manufacturer, 20 );
    memcpy( &env.ProducNo, info->producno, 20 );
    env.PAN_Topology	    = 0x09;
    env.PAN_Protocol_Stack  = 0x09;

    rtrn = sender( tcp, &env, sizeof( Struct_PAN_Registration ) );

    return rtrn;
}

int SensorNode_Description_Registration( int *tcp, READENV *info ) 
{
    int rtrn = 0;
    Struct_SensorNode_Registration env;

    env.SensorNode.Message_Id	    = 0x0005;
    env.SensorNode.Length	    = 0x004B;
    env.SensorNode.GateNode_Id	    = info->gatenode;
    env.SensorNode.PAN_Id	    = info->pan;
    env.SensorNode.SensorNode_Id   = info->sensornode;

    memset( env.Global_Id, 0, 12 );
    env.Global_Id[0] = 71;
    env.Monitoring_Mode = 0x01;
    env.Monitoring_Period = 0x000000FF;

    memcpy( &env.Manufacturer, info->manufacturer, 20 );
    memcpy( &env.ProducNo, info->producno, 20 );
    env.Longitude		    = 0;
    env.Latitude		    = 0;
    env.Altitude		    = 0;


    rtrn = sender( tcp, &env, sizeof( Struct_SensorNode_Registration ) );

    return rtrn;
}

int Transducer_Description_Registration( int *tcp, READENV *info, sqlite3 *pSQLite3 )
{
    int count;
    UINT16 rtrn;

    SQLite3Data data;
    char *query;

    int selectOffset = 9;
    int selectCount = 0;
    int i;
    int sendSize[256];
    Struct_Transducer_Registration env[256];

    float fData = 0;

    memset( env, 0, sizeof( Struct_Transducer_Registration ) * 256 ); 
    memset( sendSize, 0, sizeof( int  ) * 256 ); 

    query = sqlite3_mprintf("select count(*) from transducerinfo");
    data = IoT_sqlite3_select( pSQLite3, query );
    if( data.size > 0 )
    {
	selectCount = atoi(data.data[1]);
	printf("data.size = %d, selectCount = %d\n", data.size, selectCount );

    }



    query = sqlite3_mprintf("select * from transducerinfo");
    data = IoT_sqlite3_select( pSQLite3, query );
    printf(" select size = %ld\n", data.size );
    if( data.size > 0 )
    {
	//count = ( data.size - 9 ) / selectCount;

	//for( i = 0; i < count; i++ )
	for( i = 0; i < selectCount; i++ )
	{
	    env[i].Transducer.Message_Id	    = 0x0007;
	    env[i].Transducer.GateNode_Id	    = info->gatenode;
	    env[i].Transducer.PAN_Id		    = info->pan;
	    env[i].Transducer.SensorNode_Id	    = info->sensornode;

	    env[i].Transducer.Transducer_Id	    = atoi(data.data[selectOffset++]);

	    memcpy( &env[i].Manufacturer, info->manufacturer, 20 );
	    memcpy( &env[i].ProducNo, info->producno, 20 );


/*
unsigned long int htonl(unsigned long int hostlong);
unsigned short int htons(unsigned short int hostshort);

unsigned long int ntohl(unsigned long int netlong);
unsigned short int ntohs(unsigned short int netshort);
*/

	    env[i].Min = atoi(data.data[selectOffset++]);
	    env[i].Max = atoi(data.data[selectOffset++]);
	    //fData = atoi(data.data[selectOffset++]);
	    //env[i].Max = 0x000000ff & (fData >> 24);

	    //printf("Min %f / Max %f\n", env[i].Min, env[i].Max );
	    //env[i].Min = atoi(data.data[selectOffset++]);
	    //env[i].Max = atoi(data.data[selectOffset++]);
	    env[i].Offset = atoi(data.data[selectOffset++]);
	    env[i].Level = atoi(data.data[selectOffset++]);
	    env[i].TransducerType = atoi(data.data[selectOffset++]);
	    env[i].DataType = atoi(data.data[selectOffset++]);


	    env[i].DataUnitLength = atoi(data.data[selectOffset++]);


	    env[i].Transducer.Length		    = 0x0043 + env[i].DataUnitLength;
	    sendSize[i] = env[i].Transducer.Length + 4;
	    memcpy( &env[i].DataUnit, data.data[selectOffset++], env[i].DataUnitLength );
	    //printf("size = %d\n",  sendSize[i] );

	}
    }
    else
	selectCount = 0;

    sqlite3_free( query );
    SQLITE_SAFE_FREE( query )
	sqlite3_free_table( data.data );
    SQLITE_SAFE_FREE( data.data )

	while( selectCount ) 
	{

	    //printf(" transducer count %d, packet length %d, test size %d\n", count, env[i].Transducer.Length, size );
	    rtrn = sender( tcp, &env[selectCount-1], sendSize[selectCount-1] );
	    if( rtrn == 0 )
	    {
		selectCount--;
	    }
	    else
	    {
		rtrn = -1;
		break;
	    }
	}

    return rtrn;
}


int Transducer_Registration( int *tcp, UINT16 *transducerId )
{
    UINT8 rtrn;
    UINT8 sendBuffer[128];

    //memcpy( sendBuffer, transducerId, sizeof( UINT16 ) );
    /*
       sendBuffer[0] = (UINT8)(transducerId  >> 0);
       sendBuffer[1] = (UINT8)(transducerId  >> 8);
     */

    rtrn = send(*tcp, transducerId, sizeof( UINT16 ), MSG_NOSIGNAL);
    printf("send length = %d\n", rtrn);
    if( rtrn == -1 )
    {
	return -1;
    }

    return 0;
}

