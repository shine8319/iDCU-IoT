#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <sys/types.h>
#include <sys/ipc.h>
#include <sys/msg.h>

#define  	BUFF_SIZE   1024
#define		M2M_COMMAND		0
#define		USN_RETURN		1
#define		DI_PORT			6

typedef struct {
	long  data_type;
	int   data_num;
	char  data_buff[BUFF_SIZE];
} t_data;

typedef struct {
	char 			wo[DI_PORT];
	char 			curWo[DI_PORT];
	unsigned int	cnt[DI_PORT];
	unsigned int	curCnt[DI_PORT];
} t_node;

typedef struct { 
	unsigned char nodeid; 
	unsigned char group; 
	unsigned char seq; 
	unsigned char wo[DI_PORT]; 
	unsigned int cnt[DI_PORT]; 
} t_getNode; 


int main( void)
{
	int      msqid;
	int 		i;
	int 		id = 0;
	t_data   data;
	t_node	value[254];
	t_getNode	node;

	memset( value, 0, (sizeof( t_node ) * 254) ); 

	if ( -1 == ( msqid = msgget( (key_t)1111, IPC_CREAT | 0666)))
	{
		perror( "msgget() ½ÇÆÐ");
		exit( 1);
	}

	while( 1 )
	{

		if( -1 == msgrcv( msqid, &data, sizeof( t_data) - sizeof( long), 0, IPC_NOWAIT) )
		{
			printf("Empty Q\n");
			sleep(1);
		}
		else
		{
			memcpy( &node, data.data_buff, 33 );
			/*
			printf("NODE%d; GROUP%d; SEQ%d; ", node.nodeid, node.group, node.seq);
			for( i = 0; i < DI_PORT; i++ )
			{
				printf("WO%d:%d; CNT%d:%d; ", i+1, node.wo[i], i+1, node.cnt[i] );
			}
			printf("\n");
			*/

			id = node.nodeid;
			memcpy( value[id].curWo, node.wo, DI_PORT );
			memcpy( value[id].curCnt, node.cnt, 4*DI_PORT );


			for( i = 0; i < DI_PORT; i++ )
			{

				if( value[id].cnt[i] != value[id].curCnt[i] )
				{
					printf("NodeID %d Port %d : %d - > %d Count Change\n", id, i+1, value[id].cnt[i], value[id].curCnt[i]);
					value[id].cnt[i] = value[id].curCnt[i];
				}



				if( value[id].wo[i] != value[id].curWo[i] )
				{
					printf("NodeID %d Port %d : %d -> %d Wo Change\n", id, i+1, value[id].wo[i], value[id].curWo[i]);
					value[id].wo[i] = value[id].curWo[i];

				}

			}

		}


	}
}
