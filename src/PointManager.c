#include <stdio.h> 
#include <stdio.h> 
#include <string.h> 
#include <unistd.h>
#include <sys/ipc.h>
#include <sys/shm.h>
#include <signal.h>

#include "./include/expat.h" 
#include "./include/PointManager.h"

#define DEBUG

extern NODEINFO pointparser(const char *path);
extern int removeSharedMem( char nodeid, const void *id );

unsigned char user_quit = 0;

void quitsignal(int sig) 
{
	user_quit = 1;
}


int main(int argc, char **argv) { 

	int i;
	int rtrn;
	NODEINFO tag;
	POINTINFO point[32];

	(void)signal(SIGINT, quitsignal);
	printf("Press <ctrl+c> to user_quit. \n");

	tag = pointparser("usn_node_info.xml");

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

	printf("%d\n", sizeof(point));

	for( i = 0; i < tag.getPointSize; i++)
	{

		if ( -1 == ( point[i].id = shmget( (key_t)tag.id[i], tag.size[i], IPC_CREAT|0666)))
		{
			printf( "공유 메모리 생성 실패\n");
			return -1;
		}

	}

	for( i = 0; i < tag.getPointSize; i++)
	{
		rtrn = removeSharedMem( tag.id[i], point[i].id );
		if( rtrn == 0 )
			printf("OK\n");
	}

	return 0; 
}
