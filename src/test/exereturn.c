#include <stdio.h>

#define  BUFF_SIZE   1024

int main( void)
{
    char  buff[BUFF_SIZE];
    FILE *fp;

    fp = popen( "dpkg -s idcu-iot | grep Version", "r");
    if ( NULL == fp)
    {
	perror( "popen() 실패");
	return -1;
    }

    while( fgets( buff, BUFF_SIZE, fp) )
	printf( "%s", buff);

    pclose( fp);

    return 0;
}
