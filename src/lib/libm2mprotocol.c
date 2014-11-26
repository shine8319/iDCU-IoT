#include <stdio.h>
#include <sys/types.h>
#include <sys/wait.h>
#include <arpa/inet.h>
#include "../include/M2MManager.h"
#include "../include/sqlite3.h"

int TimeSync( QUERYTIME time );
int DeleteSensorData( sqlite3 *pSQLite3, int keepdata );
int ReturnMsgToClient( void* socket_fd, QUERYTIME time, char type, char result );

int M2MProtocol( void* socket_fd, sqlite3 *pSQLite3, QUERYTIME time, char type, int keepdata ) 
{
	int rtrn; 

	switch( type )
	{
		case 'T':
			printf("TimeSync Start~~~~~~~~~~~~\n");
			rtrn = TimeSync( time );
			printf("TimeSync Return Value %d \n", rtrn);
			ReturnMsgToClient( socket_fd, time, type, rtrn );
			if( rtrn != -1 )
				DeleteSensorData( pSQLite3, keepdata );
			break;
		default:
			break;
	}

	return 0;

}

int ReturnMsgToClient( void* socket_fd, QUERYTIME time, char type, char result )
{
	int send_size;
	unsigned char send_buff[1024];
	int sock = (int)socket_fd;


	send_buff[0] = 0xFA;
	send_buff[1] = 0xFB;

	send_buff[2] = time.year;
	send_buff[3] = time.month;
	send_buff[4] = time.day;
	send_buff[5] = time.hour;
	send_buff[6] = time.minute;

	send_buff[7] = type; 
	send_buff[8] = 0x0; 

	if( result == 0 )
	{
		send_buff[9]	= 'O'; 
		send_buff[10]	= 'K'; 
		send_buff[11]	= 'A'; 
		send_buff[12]	= 'Y'; 
	}
	else
	{
		send_buff[9]	= 'F'; 
		send_buff[10]	= 'A'; 
		send_buff[11]	= 'I'; 
		send_buff[12]	= 'L'; 
	}

	send_buff[13] = 0xFF;
	send_buff[14] = 0xFE;

	send_size = send( sock, &send_buff, 15, MSG_NOSIGNAL );
	printf("send data size : %d\n", send_size );
	if( send_size == -1 )
	{
		printf("Send Error.... ");
	}
	
	return 0;

}


int TimeSync( QUERYTIME time )
{

		char settime[12]; 
		int pid;
		int status;
		int state;
		pid_t child;
		int _2pid;
		int _2state;
		pid_t _2child;
		int rtrn;


		sprintf(settime, "%02d%02d%02d%02d20%02d", 
				time.month,
				time.day,
				time.hour,
				time.minute,
				time.year );

		if( time.year < 12 || time.year > 99 
				|| time.month < 1 || time.month > 12 
				|| time.day < 1 || time.day > 31 
				|| time.hour > 24 
				|| time.minute > 59 )
		{
			printf(" Out of Set time \n ");
			return -1;
		}
			
		printf("request set time = %s\n", settime);
		pid = fork();
		if( pid < 0 )
		{
			perror("fork error : ");
			rtrn = -1;
		}
		if( pid == 0 )
		{
			printf("in child\n");
			execl("/bin/date", "/bin/date", settime, NULL);
			exit(0);
		}
		else
		{
			do {
				child = waitpid(-1, &state, WNOHANG);
			} while(child == 0);
			printf("Parent : wait(%d)\n", pid);
			printf("Child process id = %d, return value = %d \n", child, WEXITSTATUS(state));
			rtrn = 0;
		}

		_2pid = fork();
		if( _2pid < 0 )
		{
			perror("fork error : ");
			rtrn = -1;
		}
		if( _2pid == 0 )
		{
			printf("in 2child\n");
			execl("/sbin/hwclock", "/sbin/hwclock", "-w", NULL);
			exit(0);
		}
		else
		{
			do {
				_2child = waitpid(-1, &_2state, WNOHANG);
			} while(_2child == 0);
			printf("Parent : wait(%d)\n", _2pid);
			printf("Child process id = %d, return value = %d \n", _2child, WEXITSTATUS(_2state));
			rtrn = 0;
		}



		return rtrn;

}

int DeleteSensorData( sqlite3 *pSQLite3, int keepdata )
{
	int rst;
	char *szErrMsg;
	char *query;

	//printf("(int)dev.expiretime[0] = %d\n", (int)dev.expiretime[0]);

	//query = sqlite3_mprintf("DELETE from tb_th1000_data where strftime('%%Y-%%m-%%d', datetime) < date('now', '-%d days')",
	query = sqlite3_mprintf("DELETE from dabom-device where date < date('now', '-%d days')",
			keepdata );
	printf("DELETE -%d days\n", keepdata);

	rst = sqlite3_exec(
			pSQLite3,
			query,
			0,
			0,
			&szErrMsg);

	if( rst != SQLITE_OK )
	{
		printf("DELETE fail!! : %s\n", szErrMsg);
		sqlite3_free( szErrMsg );
		if( szErrMsg != NULL )
			szErrMsg = NULL;
	}

	sqlite3_free( query);		// mem free
	if( query != NULL )
		query = NULL;


	return 0;
}



