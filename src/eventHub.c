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

#include "include/iDCU.h"
#include "include/ETRI.h"
#include "include/configRW.h"
#include "include/libpointparser.h"
#include "include/hiredis/hiredis.h"
#include "include/hiredis/async.h"
#include "include/hiredis/adaters/libevent.h"
#include "include/hexString.h"

#define EVENT_Q_SIZE 256
#define BUFFER_SIZE 1024

static void *thread_insert(void *arg);
static pthread_t threads;

static char tagID[EVENT_Q_SIZE][BUFFER_SIZE];
static char strPac[EVENT_Q_SIZE][BUFFER_SIZE*2];
//static unsigned char strPac[EVENT_Q_SIZE][BUFFER_SIZE*2];
//char redisValue[2048];
static unsigned char *teststrPac;
static UINT8 eventCount;
static UINT8 eventIn;
static UINT8 eventOut;


static READENV env;

static void onMessage(redisAsyncContext *c, void *reply, void *privdata) {

    int status, offset;
    int i,j;
    redisReply *r = reply;
    Struct_SensingValueReport pac;
    time_t  transferTime, sensingTime;
    unsigned char savePac[1024];


    if (reply == NULL) return;

    if (r->type == REDIS_REPLY_ARRAY) {
	/*
	   for (j = 0; j < r->elements; j++) {
	   printf("%u) %s\n", j, r->element[j]->str);

	   }
	 */

	if (strcasecmp(r->element[0]->str,"message") == 0) {

	    //redisAsyncFree(c);
	    //r2= redisCommand(c, "SET %s %s", r->element[1]->str, r->element[2]->str);
	    //freeReplyObject(r);
	    //printf("type:%d / integer:%lld\n", r2->type, r2->integer);
	    //printf("len:%d / str:%s\n", r2->len, r2->str);
	    //printf("elements:%d\n", r2->elements );


	    //printf("%d message~~\n", status);
	    //freeReplyObject(reply);

	    if( eventCount < EVENT_Q_SIZE )
	    {
		pac.Message_Id		= 0x09;
		pac.Length		= (SENSING_VALUE_REPORT_HEAD_SIZE-4) + r->element[2]->len;
		pac.Command_Id		= 0xFFFFFFFF;
		pac.GateNode_Id		= env.gatenode;
		pac.PAN_Id			= env.pan;
		pac.SensorNode_Id		= env.sensornode; 
		/*
		   printf(" gata %d, pan %d, node %d\n", pac.GateNode_Id
		   ,pac.PAN_Id
		   ,pac.SensorNode_Id );
		 */

		time(&transferTime);
		pac.Transfer_Time		= transferTime;

		//printf("[%s] recv Len %d / Str %s\n",
		//printf("[%s] recv Len %d\n",
			//r->element[1]->str, r->element[2]->len );

		offset = 0;
		memset( savePac, 0, sizeof( savePac ) );

		memcpy( savePac+offset, &pac.Message_Id, sizeof( pac.Message_Id ) );
		offset += sizeof( pac.Message_Id );
		memcpy( savePac+offset, &pac.Length, sizeof( pac.Length ) );
		offset += sizeof( pac.Length );
		memcpy( savePac+offset, &pac.Command_Id, sizeof( pac.Command_Id ) );
		offset += sizeof( pac.Command_Id );
		memcpy( savePac+offset, &pac.GateNode_Id, sizeof( pac.GateNode_Id ) );
		offset += sizeof( pac.GateNode_Id );
		memcpy( savePac+offset, &pac.PAN_Id, sizeof( pac.PAN_Id ) );
		offset += sizeof( pac.PAN_Id );
		memcpy( savePac+offset, &pac.SensorNode_Id, sizeof( pac.SensorNode_Id ) );
		offset += sizeof( pac.SensorNode_Id );
		memcpy( savePac+offset, &pac.Transfer_Time, sizeof( pac.Transfer_Time ) );
		offset += sizeof( pac.Transfer_Time );

		memset( tagID[eventIn], 0, BUFFER_SIZE );
		memset( strPac[eventIn], 0, BUFFER_SIZE*2 );

		for( i = 0; i < offset; i++ ) 
		    sprintf(strPac[eventIn]+(i*2), "%02X", savePac[i]);

		sprintf(strPac[eventIn]+strlen( strPac[eventIn] ), "%s", r->element[2]->str);
		sprintf(tagID[eventIn], "%s", r->element[1]->str);

		eventCount++;
		if( ++eventIn >= EVENT_Q_SIZE ) eventIn = 0;

	    }
	    else
	    {
		// var init
		eventCount = 0;
		eventIn = 0;
		eventOut = 0;
		memset( strPac, 0, sizeof( strPac ) );

		writeLog( "/work/smart/log", "[onMessage] full Q" );
	    }


	}
    }
}

int main (int argc, char **argv) {
    signal(SIGPIPE, SIG_IGN);
    struct event_base *base = event_base_new();

    char th_data[256];
    char tagCount[256];
    int i, stringOffset = 0;

    NODEINFO xmlinfo;

    memset( &env, 0, sizeof( READENV ) );
    memset( tagCount, 0, 256 );

    ReadEnvConfig("/work/smart/reg/env.reg", &env );

    xmlinfo = pointparser("/work/smart/tag_info.xml");


    sprintf(tagCount+stringOffset, "SUBSCRIBE" ); 
    stringOffset += strlen(tagCount);

    for( i = 0; i < xmlinfo.getPointSize; i++ )
    {
	sprintf(tagCount+stringOffset, " %s", xmlinfo.tag[i].id ); 
	stringOffset += 1 + strlen(xmlinfo.tag[i].id);
    }

    if( xmlinfo.getPointSize == 0 ) {

	printf("no exist tag\n");
	return; 
    } else {
	printf("%s\n", tagCount);
    }


    redisAsyncContext *c = redisAsyncConnect("127.0.0.1", 6379);

    if (c->err) {
	printf("error: %s\n", c->errstr);
	return 1;
    }

    //memcpy( th_data, (void *)c, sizeof(redisAsyncContext) );
    if( pthread_create(&threads, NULL, &thread_insert, NULL) == -1 )
    {
	//writeLog("/work/smart/log", "[tagServer] error pthread_create method" );
    }


    redisLibeventAttach(c, base);
    redisAsyncCommand(c, onMessage, NULL, tagCount );
    //redisAsyncCommand(c, onMessage, NULL, "SUBSCRIBE 2");
    event_base_dispatch(base);
    return 0;
}

static void *thread_insert(void *arg)
{


    unsigned char insertPac[EVENT_Q_SIZE][BUFFER_SIZE*2];
    int rtrn,i;

    redisReply *reply;


    static redisContext *c;

    const char *hostname = "127.0.0.1";
    //const char *hostname = "10.1.0.6";
    int port = 6379;

    struct timeval timeout = { 1, 500000 }; // 1.5 seconds
    c = redisConnectWithTimeout(hostname, port, timeout);
    if (c == NULL || c->err) {
	if (c) {
	    printf("Connection error: %s\n", c->errstr);
	    redisFree(c);
	} else {
	    printf("Connection error: can't allocate redis context\n");
	}
	exit(1);
    }



    while(1)
    {

	if( eventCount != 0)
	{

	    printf("[%s] Insert Length %d\n", tagID[eventOut], strlen(strPac[eventOut]));

	    reply = redisCommand(c,"lpush tag1 %s", 
		    strPac[eventOut]);
	    printf("type:%d / index:%lld\n", reply->type, reply->integer);
	    printf("len:%d / str:%s\n", reply->len, reply->str);
	    printf("elements:%d\n", reply->elements );


	    rtrn = reply->integer;
	    freeReplyObject(reply);


	    if (strcasecmp(tagID[eventOut],"1") == 0) {
		writeLogV2( "/work/smart/log", "eventHub", "[%s] %s", tagID[eventOut], strPac[eventOut]);
	    }

	    if( rtrn > 0 )
	    {
		// success
		eventCount--;
		if( ++eventOut >= EVENT_Q_SIZE ) eventOut = 0;
	    }
	    else
		writeLog( "/work/smart/log", "[eventHub] fail insert" );


	    //usleep(10000);	// 10ms

	}
	else
	{
	    //printf("no Data (eventCount %d)\n", eventCount );
	    usleep(500000);	// 500ms
	}

    }

}


/*
   static redisContext *c;
   static redisReply *reply;
   static int tagID;

   int main()
   {
   const char *hostname = "127.0.0.1";
   int port = 6379;
   int i;

   struct timeval timeout = { 1, 500000 }; // 1.5 seconds
   c = redisConnectWithTimeout(hostname, port, timeout);
   if (c == NULL || c->err) {
   if (c) {
   printf("Connection error: %s\n", c->errstr);
   redisFree(c);
   } else {
   printf("Connection error: can't allocate redis context\n");
   }
   exit(1);
   }

   reply = redisCommand(c,"SUBSCRIBE 1 2 3 4 5 6 7 8 9 10");
   printf("type:%d / integer:%lld\n", reply->type, reply->integer);
   printf("len:%d / str:%s\n", reply->len, reply->str);
   printf("elements:%d\n", reply->elements );
   for( i = 0; i < reply->elements; i++ )
   {
//printf("[%d] type: %d integer:%lld\n", i, reply->element[i]->type, reply->element[i]->integer);
printf("[%d] len: %d str:%s\n", i, reply->element[i]->len, reply->element[i]->str);
//printf("[%d] elements: %d\n", i, reply->element[i]->elements);
}

freeReplyObject(reply);
printf("while in \n");
struct timeval val;
struct tm *t;


while(redisGetReply(c,&reply) == REDIS_OK) {

//printf("type:%d / integer:%lld\n", reply->type, reply->integer);
//printf("len:%d / str:%s\n", reply->len, reply->str);
printf("elements:%d => ", reply->elements );
gettimeofday(&val, NULL);
t = localtime(&val.tv_sec);



printf("[%s] - %04d/%02d/%02d %02d:%02d:%02d.%03ld \n", reply->element[1]->str
, t->tm_year + 1900,
t->tm_mon + 1,
t->tm_mday,
t->tm_hour,
t->tm_min,
t->tm_sec,
val.tv_usec/1000


);
// consume message
freeReplyObject(reply);



}

return 0;
}
 */
