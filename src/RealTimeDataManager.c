#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <sys/ipc.h>
#include <sys/msg.h>
#include <time.h>
#include <sys/reboot.h>


#include "./include/RealTimeDataManager.h"
#include "./include/iDCU.h"
#include "./include/sqlite3.h"
#include "./include/writeLog.h"
#include "./include/TCPSocket.h"

extern int el1000_sqlite3_open( char *path, sqlite3 **pSQLite3 );
extern int el1000_sqlite3_customize( sqlite3 **pSQLite3 );
extern int el1000_sqlite3_close( sqlite3 **pSQLite3 );
extern int el1000_sqlite3_transaction( sqlite3 **pSQLite3, char status );
extern Return_Selectdata el1000_sqlite3_select( sqlite3 *pSQLite3, char *query );
extern int InsertAustemNodeData( sqlite3 **pSQLite3, t_getNode	node );
extern int InsertAustemNodeStatusData( sqlite3 **pSQLite3, t_getNode	node );
extern int UpdateAustemNodeData( sqlite3 **pSQLite3, t_getNode	node );
static int busy(void *handle, int nTry);
int commandDeleteNode( sqlite3 *pSQLite3, unsigned char nodeid );
int commandUpdateNode( sqlite3 *pSQLite3, unsigned char nodeid, unsigned char changenodeid, unsigned char groupid );

int tcp;
int main( void)
{
	int      msqid;
	int      eventid;
	int 		i,j;
	int 		id = 0;
	int			rtrn;
	char		cntOffset = 9;
	t_data   data;
	t_data   eventData;
	t_node	value[256];
	t_getNode	node;
	t_getNode	lossNode;
	int loss[DI_PORT];
	int lossOffset = 0;
	char lossCheckStart[256];

	// sqlite
	int rc;
	sqlite3 *pSQLite3;
	int transaction = 0;
	//NODELIST nodelist;
	unsigned char nodelist[256];
	char *query;
	Return_Selectdata selectLastData;
	int	getPortOffset = 0;
	//int	dataChanged = 0;
	int	packetErr = 0;
	unsigned char checkSumCompare = 0;
	UINT8 sendBuffer[1024];

	//clock_t start,end;
	//double msec;

	memset( &data, 0, sizeof(t_data) );
	memset( &eventData, 0, sizeof(t_data) );
	memset( value, 0, (sizeof( t_node ) * 256) ); 
	memset( nodelist, 0, 256 ); 
	memset( lossCheckStart, 0, 256 ); 
	memset( loss, 0, (sizeof(int) * DI_PORT) ); 

	memset( sendBuffer, 0, 1024 );

	tcp = TCPClient( "127.0.0.1", "50007");

	if ( -1 == ( msqid = msgget( (key_t)1, IPC_CREAT | 0666)))
	{
		writeLog( ".", "error msgget() msqid" );
		return -1;
	}

	while( 1 )
	{

		memset( &data, 0, sizeof(t_data) );
		if( -1 == msgrcv( msqid, &data, sizeof( t_data) - sizeof( long), 0, IPC_NOWAIT) )
		{
			//printf("Empty Q\n");
			sleep(1);
		}
		else
		{
			printf("recv size = %d, type = %ld\n", data.data_num, data.data_type);
				//printf("%s\n", data.data_buff);
			for( i = 0; i < data.data_num; i++ )
				printf("%02X ", data.data_buff[i]);
			printf("\n");
			memcpy( sendBuffer, data.data_buff, data.data_num );
			//rtrn = sendto( tcp, data.data_buff, data.data_num, 0, NULL, NULL);
			rtrn = send(tcp, sendBuffer, data.data_num, MSG_NOSIGNAL);
			printf("send length = %d\n", rtrn);
			if( rtrn == -1 )
			{
			    close(tcp);
			    tcp = TCPClient( "127.0.0.1", "50007");
			}

		}


	}
}

int commandUpdateNode( sqlite3 *pSQLite3, unsigned char nodeid, unsigned char changenodeid, unsigned char groupid )
{
	char* updateQuery;

	updateQuery = sqlite3_mprintf("update tb_comm_status set nodeid = '%d', groupid = '%d' where nodeid = '%d'",
			changenodeid,
			groupid,
			nodeid
			);

	el1000_sqlite3_delete( pSQLite3, updateQuery );

	sqlite3_free( updateQuery );
	SQLITE_SAFE_FREE( updateQuery )

	return 0;
}

int commandDeleteNode( sqlite3 *pSQLite3, unsigned char nodeid )
{
	char* delQuery;

	delQuery = sqlite3_mprintf("delete from tb_comm_status where nodeid = '%d'",
			nodeid);

	el1000_sqlite3_delete( pSQLite3, delQuery );

	sqlite3_free( delQuery );
	SQLITE_SAFE_FREE( delQuery )

	return 0;
}

static int busy(void *handle, int nTry)
{
	printf("[%d] busy handler is called\n", nTry);

	//writeLog( "DB busy : call sqlite3_busy_handler" );
	return 0;

}
