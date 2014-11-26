#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <time.h>
#include <sys/types.h>

typedef struct TMSTRUCT
{
    struct tm *tm_data;
} TMSTRUCT;

int getDiffTime( struct tm callTime )
{
    time_t tm_nd;
    time_t cur_time;
    int diff_time;

    tm_nd = mktime( &callTime);
    //printf( "tm_nd %ld\n", tm_nd );

    if( tm_nd < 0 )
	return -1;

    time(&cur_time);
    //printf( "tm_st %ld\n", cur_time);

    //printf("\n");

    diff_time = cur_time - tm_nd;

    //printf("difftime %d\n", diff_time );

    return diff_time;
}
