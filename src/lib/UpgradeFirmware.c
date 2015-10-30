#include <stdio.h>
#include <string.h>
#include <stdlib.h>


#define BUFF_SIZE   1024
int s_getFirmware (char * out)
{
    char buff[BUFF_SIZE];
    FILE *fp;
    char *ptr;

    fp = popen( "dpkg -s idcu-iot | grep Version", "r");
    if ( NULL == fp)
    {
	perror( "popen() 실패");
	return -1;
    }

    while( fgets( buff, BUFF_SIZE, fp) )
	printf( "%s", buff);


    if( buff[0] == 'V' && 
	buff[1] == 'e' &&
	buff[2] == 'r' &&
	buff[3] == 's' &&
	buff[4] == 'i' &&
	buff[5] == 'o' &&
	buff[6] == 'n' )
    {
	ptr = strtok( buff, " ");
	//printf("%s\n", ptr);
	while( ptr = strtok( NULL, " "))
	{
	    //printf( "%s\n", ptr); 
	    strcpy( out, ptr);
	    printf( "%s\n", out); 
	}

	//out = (char *)malloc(strlen(ptr));

	//printf("hi\n");

	pclose( fp);

	return strlen(out);

    }
    else
    {
	return 0;
    }


}


