#include <stdio.h> 
#include <stdlib.h> 
#include <string.h> 
#include <unistd.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <errno.h>
#include <time.h>

#include "../include/iDCU.h"

void writeLog( UINT8 *path, UINT8 *str )
{
    FILE    *fp_log;
    struct timeval val;
    struct tm *t;
    char     buff[10240];
    char     devName[512];
    int	    pid;
    
    memset( devName, 0, sizeof( devName ));
    
    gettimeofday(&val, NULL);
    t = localtime(&val.tv_sec);

    sprintf( devName, "%s/%04d-%02d-%02d.log",
	    path,
	    t->tm_year + 1900,
	    t->tm_mon + 1,
	    t->tm_mday );


    fp_log = fopen( devName, "a");
    if( !fp_log )
    {

	printf("no exist folder\n");

	pid = fork();
	if( pid < 0 )
	{
	    perror("fork error : ");
	}

	if( pid == 0 )
	{
	    execl("/bin/mkdir", "/bin/mkdir", path, NULL);
	    exit(0);
	}
	else
	{
	    return;
	}
    }

    
    sprintf( buff, "%04d/%02d/%02d %02d:%02d:%02d.%03ld - %s\n",
	    t->tm_year + 1900,
	    t->tm_mon + 1,
	    t->tm_mday,
	    t->tm_hour,
	    t->tm_min,
	    t->tm_sec,
	    val.tv_usec/1000,
	    str);

    fwrite( buff, 1, strlen(buff), fp_log );

    fclose( fp_log );
}

void writeLogV2( UINT8 *path, UINT8 *filename, UINT8 *str )
{
    FILE    *fp_log;
    struct timeval val;
    struct tm *t;
    char     buff[10240];
    char     devName[512];
    int	    pid;
    
    memset( devName, 0, sizeof( devName ));
    
    gettimeofday(&val, NULL);
    t = localtime(&val.tv_sec);

    sprintf( devName, "%s/%s_%04d-%02d-%02d.log",
	    path,
	    filename,
	    t->tm_year + 1900,
	    t->tm_mon + 1,
	    t->tm_mday );


    fp_log = fopen( devName, "a");
    if( !fp_log )
    {

	printf("no exist folder\n");

	pid = fork();
	if( pid < 0 )
	{
	    perror("fork error : ");
	}

	if( pid == 0 )
	{
	    execl("/bin/mkdir", "/bin/mkdir", path, NULL);
	    exit(0);
	}
	else
	{
	    return;
	}
    }

    
    sprintf( buff, "%04d/%02d/%02d %02d:%02d:%02d.%03ld - %s\n",
	    t->tm_year + 1900,
	    t->tm_mon + 1,
	    t->tm_mday,
	    t->tm_hour,
	    t->tm_min,
	    t->tm_sec,
	    val.tv_usec/1000,
	    str);

    fwrite( buff, 1, strlen(buff), fp_log );

    fclose( fp_log );

}
