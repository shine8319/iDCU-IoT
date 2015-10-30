#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <sys/types.h>
#include <sys/ipc.h>
#include <sys/msg.h>
#include <time.h>
#include <sys/reboot.h>


#define LOGPATH "/work/log/RealTimeDataManager"
#include "./include/sqlite3.h"
#include "./include/RealTimeDataManager.h"
#include "./include/iDCU.h"

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

	//clock_t start,end;
	//double msec;

	memset( &data, 0, sizeof(t_data) );
	memset( &eventData, 0, sizeof(t_data) );
	memset( value, 0, (sizeof( t_node ) * 256) ); 
	memset( nodelist, 0, 256 ); 
	memset( lossCheckStart, 0, 256 ); 
	memset( loss, 0, (sizeof(int) * DI_PORT) ); 

	//sleep(4);

	if ( -1 == ( msqid = msgget( (key_t)1111, IPC_CREAT | 0666)))
	{
		//writeLog( "error msgget() msqid" );
		writeLogV2(LOGPATH, "[RealTimeDataManager]", "error msgget() msqid\n");
		//perror( "msgget() 실패");
		exit( 1);
	}
	if( -1 == ( eventid = msgget( (key_t)3333, IPC_CREAT | 0666)))
	{
		//writeLog( "error msgget() eventid" );
		writeLogV2(LOGPATH, "[RealTimeDataManager]", "error msgget() eventid\n");
		//perror( "msgget() 실패");
		return -1;
	}


	/************** DB connect.. ***********************/
	// DB Open
	rc = el1000_sqlite3_open( DBPATH, &pSQLite3 );
	//rc = el1000_sqlite3_open( argv[1], &pSQLite3 );
	if( rc != 0 )
	{
		//writeLog( "error DB Open" );
		writeLogV2(LOGPATH, "[RealTimeDataManager]", "error DB Open\n");
		return -1;
	}
	else
	{
		printf("%s OPEN!!\n", DBPATH);
		writeLogV2(LOGPATH, "[RealTimeDataManager]", "%s OPEN!!\n", DBPATH);
		//printf("%s OPEN!!\n", argv[1]);
	}

	// DB Customize
	rc = el1000_sqlite3_customize( &pSQLite3 );
	if( rc != 0 )
	{
		//writeLog( "error DB Customize" );
		writeLogV2(LOGPATH, "[RealTimeDataManager]", "error DB Customize\n");
		return -1;
	}

	//sqlite3_busy_handler( pSQLite3, busy, NULL);
	//sqlite3_busy_timeout( pSQLite3, 3000);
	//sqlite3_busy_timeout( pSQLite3, 1000);


	/**********************************************/

	//start = clock();
	rtrn = getNodeList( pSQLite3, nodelist );
	if( rtrn > 0 )
	{
		do 
		{
			transaction = el1000_sqlite3_transaction( &pSQLite3, INSERT_BEGIN );
			/*
			if( transaction == -1 )
				writeLog( "TRANSACTION BEGIN ERROR" );
				*/
		}while( transaction );


		for( i = 0; i < rtrn; i++ )
		{
			printf( "list %d - %d\n", i+1, nodelist[i] );

			value[nodelist[i]].exist = 1;
			// get last sensing data
			//query = sqlite3_mprintf("select strftime('%%Y%%m%%d%%H%%M%%S', datetime), port1, port2, port3, port4, port5, port6 from tb_comm_log where nodeid = '%d' order by id desc limit 1",
			query = sqlite3_mprintf("select strftime('%%Y%%m%%d%%H%%M%%f', datetime), port1, port2, port3, port4, port5, port6 from tb_comm_status where nodeid = '%d' limit 1",
					nodelist[i]		
					);
			
			selectLastData = el1000_sqlite3_select( pSQLite3, query );
			printf("select size = %d\n", selectLastData.size );

			if( selectLastData.size > 0 )
			{
				for( j = selectLastData.size/2+1; j < selectLastData.size; j++ )
				{
					if( selectLastData.past_result[j] == NULL )
					{
						printf("data NULL\n");
					}
					else
					{
						value[nodelist[i]].cnt[getPortOffset] = atoi(selectLastData.past_result[j]);
					}
					printf("Port %d : %d\n", getPortOffset+1, value[nodelist[i]].cnt[getPortOffset++]);
				}
				getPortOffset = 0;

			}
			
			sqlite3_free( query );
			SQLITE_SAFE_FREE( query )
			sqlite3_free_table( selectLastData.past_result );
			SQLITE_SAFE_FREE( selectLastData.past_result )


		}
		do
		{
			transaction = el1000_sqlite3_transaction( &pSQLite3, INSERT_END ) ;
			/*
			if( transaction == -1 )
				writeLog( "TRANSACTION END ERROR" );
				*/
		} while( transaction );

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
		    /*
			printf("===>");
			for( i = 0; i < 33; i++ )
				printf("%02X ", data.data_buff[i]);
			printf("\n");
			*/

			//memcpy( &node, data.data_buff, sizeof(t_getNode) );
			checkSumCompare = 0;
	

			for( i = 0; i < data.data_num-1; i++ )
			{
				checkSumCompare += data.data_buff[i];
			}

			/*
			for( i = 0; i < data.data_num; i++ )
			{
				printf("%02X ",data.data_buff[i]);
			}
			printf("\n");
			*/

			if( (data.data_buff[data.data_num-1] != checkSumCompare) || data.data_num != 34 )
			{
				//writeLog( "error checkSum" );
				writeLogV2(LOGPATH, "[RealTimeDataManager]", "error checkSum\n");
				continue;
			}

			/*
			printf("packet length %d, checksum %d == %d(checkSumCompare)\n",
					data.data_num,
					data.data_buff[data.data_num-1],
					checkSumCompare);
					*/

			//if( data.data_num != eventData.data_buff[1] + 3 )

			node.nodeid = data.data_buff[0];
			node.group = data.data_buff[1];
			node.seq = data.data_buff[2];
			node.wo[0] = data.data_buff[3];
			node.wo[1] = data.data_buff[4];
			node.wo[2] = data.data_buff[5];
			node.wo[3] = data.data_buff[6];
			node.wo[4] = data.data_buff[7];
			node.wo[5] = data.data_buff[8];
			for( i = 0; i < DI_PORT; i++ )
			{
				node.cnt[i] = 0;

				node.cnt[i] = data.data_buff[cntOffset++] << 24;
				node.cnt[i] |= data.data_buff[cntOffset++] << 16;
				node.cnt[i] |= data.data_buff[cntOffset++] << 8;
				node.cnt[i] |= data.data_buff[cntOffset++] << 0;
				
			}
			cntOffset = 9;

			printf("node : %d, group : %d, seq : %d\n", node.nodeid, node.group, node.seq);
			printf("%d %d %d %d %d %d\n",
					node.wo[0],
					node.wo[1],
					node.wo[2],
					node.wo[3],
					node.wo[4],
					node.wo[5] );
			printf("%d %d %d %d %d %d\n",
					node.cnt[0],
					node.cnt[1],
					node.cnt[2],
					node.cnt[3],
					node.cnt[4],
					node.cnt[5] );

			id = node.nodeid;
			for( i = 0; i < DI_PORT; i++ )
			{
				value[id].curWo[i] = node.wo[i];
				value[id].curCnt[i] = node.cnt[i];
			}
			//memcpy( value[id].curWo, node.wo, DI_PORT );
			//memcpy( value[id].curCnt, node.cnt, DI_PORT*4);

			memcpy( &lossNode, &node, sizeof( t_getNode ) );

			for( i = 0; i < DI_PORT; i++ )
			{

				loss[i] = value[id].curCnt[i] - value[id].cnt[i];

				if( lossCheckStart[id] )
				{
					if( loss[i] > 100 )
					{
						printf("loss[i] > 100\n");
						node.changed[i] = 0;
						packetErr = 1;
						//dataChanged = 0;
						value[id].cnt[i] = value[id].curCnt[i];	// add 2013.11.20
					}
					else
					{

						if( value[id].cnt[i] != value[id].curCnt[i] )
						{

							printf("[%d] Check pre( %d ) == cur ( %d ) | exist %d\n", id, value[id].cnt[i] , value[id].curCnt[i], value[id].exist );
							node.changed[i] = 1;
							packetErr = 0;
							//dataChanged = 1;

							lossNode.cnt[i] = value[id].cnt[i]+1;

							value[id].cnt[i] = value[id].curCnt[i];
						}

					}


				}
				else
				{

						printf("New Start [%d] Check pre( %d ) == cur ( %d ) | exist %d\n", id, value[id].cnt[i] , value[id].curCnt[i], value[id].exist );
					if( value[id].cnt[i] != value[id].curCnt[i] )
					{


						node.changed[i] = 1;
						packetErr = 0;
						//dataChanged = 1;

						value[id].cnt[i] = value[id].curCnt[i];
					}
				}

			}

			// insert
			//start = clock();
			do 
			{
				transaction = el1000_sqlite3_transaction( &pSQLite3, INSERT_BEGIN );
				/*
				if( transaction == -1 )
					writeLog( "TRANSACTION BEGIN ERROR" );
					*/

			}while( transaction );

			for( i = 0; i < DI_PORT; i++ )
			{
				if( node.changed[i] )
				{
					if( lossCheckStart[id] == 1 && loss[i] > 1 )
					{
						//writeLog( "Packet Loss" );
						for( lossOffset = 1; lossOffset < loss[i]; lossOffset++ )
						{

							//printf("loss[%d] = %d\n", i, loss[i] );
							lossNode.port = i;
							InsertAustemNodeData( &pSQLite3, lossNode );
							lossNode.cnt[i]++; 
						}
						loss[i] = 0;
					}
					node.port = i;
					InsertAustemNodeData( &pSQLite3, node );
				}
				node.changed[i] = 0;
			}
			lossCheckStart[id] = 1;
			//memset( &lossNode, 0, sizeof( t_getNode ) );

			if( !packetErr && value[id].exist )
			{
				//printf("Update!!\n");
				UpdateAustemNodeData( &pSQLite3, node );
			}
			else if( !packetErr )
			{
				//printf("Insert!!\n");
				value[id].exist = 1;
				InsertAustemNodeStatusData( &pSQLite3, node );
			}
			printf("PacketErr %d\n", packetErr);
			packetErr = 0;

			do
			{
				transaction = el1000_sqlite3_transaction( &pSQLite3, INSERT_END ) ;
				/*
				if( transaction == -1 )
					writeLog( "TRANSACTION END ERROR" );
					*/

			} while( transaction );

		}


		memset( &eventData, 0, sizeof(t_data) );
		// event receive ( delete node, command reboot )
		if( -1 == msgrcv( eventid, &eventData, sizeof( t_data) - sizeof( long), 0, IPC_NOWAIT) )
		{
			// error
		}
		else
		{

			if( eventData.data_num != eventData.data_buff[1] + 3 )
				printf("packet error ( size %d == %d )\n", eventData.data_num, eventData.data_buff[1]);
			else
			{

				for( i = 0; i < eventData.data_num; i++ )
					printf("%02X ", eventData.data_buff[i]);
				printf("\n");
				switch(  eventData.data_buff[2] )
				{
					//reboot
					case 1:
						//reboot(RB_AUTOBOOT);
						execl("/sbin/reboot", "/sbin/reboot", NULL);
						break;
					//CHANGENODEINFO
					case 18:
						/*
						printf("exist Clear : %d\n", eventData.data_buff[4]);
						value[eventData.data_buff[4]].exist = 0;
						lossCheckStart[eventData.data_buff[4]] = 0;
						value[eventData.data_buff[5]].exist = 1;
						do 
						{
							transaction = el1000_sqlite3_transaction( &pSQLite3, INSERT_BEGIN );
						}while( transaction );
						commandUpdateNode( pSQLite3, eventData.data_buff[4], eventData.data_buff[5], eventData.data_buff[6] );
						do
						{
							transaction = el1000_sqlite3_transaction( &pSQLite3, INSERT_END ) ;
						} while( transaction );
						break;
						*/
					//DELETENODE
					case 4:
						printf("exist Clear : %d\n", eventData.data_buff[4]);
						value[eventData.data_buff[4]].exist = 0;
						lossCheckStart[eventData.data_buff[4]] = 0;
						do 
						{
							transaction = el1000_sqlite3_transaction( &pSQLite3, INSERT_BEGIN );
						}while( transaction );
						commandDeleteNode( pSQLite3, eventData.data_buff[4] );
						do
						{
							transaction = el1000_sqlite3_transaction( &pSQLite3, INSERT_END ) ;
						} while( transaction );

						break;
				}
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
