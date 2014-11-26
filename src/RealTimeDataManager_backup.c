#include <stdio.h> 
#include <string.h> 
#include <unistd.h>
#include <sys/ipc.h>
#include <sys/shm.h>
#include <signal.h>
#include <time.h>

#include "./include/expat.h" 
#include "./include/sqlite3.h"
#include "./include/PointManager.h"
#include "./include/RealTimeDataManager.h"

#define DEBUG
//#define DBPATH "/root/TH1000.sqlite3"
//#define DBPATH "/home/root/work/TH1000.sqlite3"


//char *DBPATH;
unsigned char user_quit = 0;

extern int el1000_sqlite3_open( char *path, sqlite3 **pSQLite3 );
extern int el1000_sqlite3_customize( sqlite3 **pSQLite3 );
extern int el1000_sqlite3_close( sqlite3 **pSQLite3 );
extern int el1000_sqlite3_transaction( sqlite3 **pSQLite3, char status );
extern NODEINFO pointparser(const char *path);
extern Return_RTDParser RTDParser( char nodeid, char *addr, char size );
extern int detachSharedMem( char type, char nodeid, const void *addr, const void *id );
extern DEVINFO deviceparser(const char *path);
extern int getNodeInfo( sqlite3 *pSQLite3, char nodeid );

void quitsignal(int sig) 
{
	user_quit = 1;
}

int main(int argc, char **argv) { 

	int i;

	NODEINFO tag;
	DEVINFO dev;
	POINTINFO point[32];

	struct shmid_ds shm_info;

	int rc;
	sqlite3 *pSQLite3;
	Return_RTDParser dispens;

	clock_t start,end;
	double msec;
	int rtrn;
	int end_transaction_rty = 0;

	//sleep(2);

	if (argc < 2) {
		printf ("%d ",argc);
		fprintf(stderr, "Usage: %s <DB Path>\n", argv[0]);
		return -1;
	}


	(void)signal(SIGINT, quitsignal);
	printf("Press <ctrl+c> to user_quit. \n");

	tag = pointparser("usn_node_info.xml");
	dev = deviceparser("RTD_info.xml");

#ifdef DEBUG
	printf("Received size = %d, %d\n", sizeof(NODEINFO), sizeof(tag));
	printf("tag.getPointSize = %d\n", tag.getPointSize);
	for( i = 0; i < tag.getPointSize; i++)
	{
		printf("tag.id[%d] = %d\n", i,tag.id[i]);
		printf("tag.type[%d] = %d\n", i,tag.type[i]);
		printf("tag.addr[%d] = %d\n", i,tag.addr[i]);
		printf("tag.size[%d] = %d\n", i,tag.size[i]);
	}
#endif

	//POINTINFO point[tag.getPointSize];
	printf("%d\n", sizeof(point));


	for( i = 0; i < tag.getPointSize; i++)
	{

		if ( -1 == ( point[i].id = shmget( (key_t)tag.id[i], tag.size[i], IPC_CREAT|0666)))
		{
			printf( "Fail create\n");

		}

		if ( ( void *)-1 == ( point[i].addr = shmat( (key_t)point[i].id, ( void *)0, 0)))
		{
			printf( "Fail Attach\n");
			return -1;
		}

	}

	/************** DB connect.. ***********************/
	//char	*szErrMsg;

	// DB Open
	//rc = el1000_sqlite3_open( DBPATH, &pSQLite3 );
	rc = el1000_sqlite3_open( argv[1], &pSQLite3 );
	if( rc != 0 )
	{
		return -1;
	}
	else
		printf("%s OPEN!!\n", argv[1]);
		//printf("%s OPEN!!\n", DBPATH);

	// DB Customize
	rc = el1000_sqlite3_customize( &pSQLite3 );
	if( rc != 0 )
		return -1;
	/**********************************************/

	//memset( &dispens, 0xFF, sizeof(Return_RTDParser) );
	for( i = 0; i < tag.getPointSize; i++ )
	{
		rtrn = getNodeInfo( pSQLite3, tag.id[i] );
		printf(" node exist = %d\n", rtrn );
		//dispens.nodeid = tag.id[i];
		if( rtrn == 0 )
			InsertNode( &pSQLite3, tag.id[i] );

	}

	while(!user_quit)
	{


		start = clock();
		memset( &dispens, 0, sizeof(Return_RTDParser));

		if( -1 != el1000_sqlite3_transaction( &pSQLite3, INSERT_BEGIN ) )
		{
			printf("BEGIN\n");
			for( i = 0; i < tag.getPointSize; i++ )
			{

				dispens = RTDParser( tag.id[i], (char *)point[i].addr, tag.size[i] );

				if( dispens.rtrn == 0 )
				{
#ifdef DEBUG
					printf("RTDParser = %d\n", dispens.rtrn );
					printf("ID:%d ", dispens.nodeid);
					//printf("GROUP:%s ", dispens.groupid);
					printf("temp:%s ", dispens.temp);
					printf("humi:%s ", dispens.humi);
					printf("lux:%s ", dispens.lux);
					printf("bat:%s", dispens.bat);
					printf("seq:%s\n\n", dispens.seq);
#endif

					dispens.alive = 1;

				}
				else
				{

					dispens.nodeid = tag.id[i];
					dispens.alive = 0;
				}
				RTDProcess( &pSQLite3, dispens );

				memset( point[i].addr, 0, tag.size[i] );
			}

			do {
				if( end_transaction_rty > 0 )
					printf("END TRANSACTION RTY = %d\n", end_transaction_rty );
				end_transaction_rty++;
			} while( ( -1 == el1000_sqlite3_transaction( &pSQLite3, INSERT_END ) ) );

			end_transaction_rty = 0;
			printf("END\n");
			end = clock();
			msec = 1000.0 * (end - start) / CLOCKS_PER_SEC;
			printf("Elapsed time : %.1f ms\n", msec);
		}

		//sleep(10);
		printf("(int)dev.processcycle[0] = %d\n", (int)dev.processcycle[0]);
		sleep((int)dev.processcycle[0]);
	}


	//int rtrn;
	
	// release shared memory
	for( i = 0; i < tag.getPointSize; i++ )
	{
		rtrn = detachSharedMem( 2, tag.id[i], point[i].addr, point[i].id );
		printf("rtrn = %d\n", rtrn);

	}

	el1000_sqlite3_close( &pSQLite3 );

	printf("[RTD_Manager] end of main loop \n");
	return 0; 
} 
