#include <stdio.h> 
#include <string.h>
#include <sys/shm.h>



int detachSharedMem( char type, char nodeid, const void *addr, const void *id )
{

	struct shmid_ds shm_info;
	char *who;

	switch( type )
	{
		case 0:
			who = "TagSrv";
			break;
		case 1:
			who = "Comm";
			break;
		case 2:
			who = "RTD";
			break;
		default:
			who = "Unknown";
			break;
	}


	if ( -1 == shmdt( (key_t)addr ))
	{
		printf("[%s] Fail detach\n", who);
		return -1;
	}

	if ( -1 == shmctl( (key_t)id, IPC_STAT, &shm_info))
	{
		printf("[%s] Fail getting shared memory info.\n", who);
		return -1;
	}
	printf("[%s] Number of process is %d ea using shared memory\n", who, shm_info.shm_nattch);
	printf("[%s] tagid:%d cpid:%d lpid:%d size:%d\n",
			who,
			nodeid,
			shm_info.shm_cpid,
			shm_info.shm_lpid,
			shm_info.shm_segsz );

	return 0;

}


int removeSharedMem( char nodeid, const void *id )
{

	if ( -1 == shmctl( (key_t)id, IPC_RMID, 0))
	{
		printf("tagid:%d Fail Remove shared memory\n", nodeid);
		return -1;
	}
	printf("tagid:%d Success Remove shared memory\n", nodeid);

	return 0;
}
