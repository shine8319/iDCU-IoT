#include <stdio.h> 
#include <stdarg.h>
#include <stdlib.h> 
#include <string.h> 
#include <unistd.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <errno.h>
#include <time.h>

#include "../include/iDCU.h"

void debugPrintf( UINT8 *debug, const char *str, ...)
{
    char     buff[10240];
    char    argBuf[10240];
    va_list ap;

    if ( strcmp(debug,"1") == 0 ) {

	va_start(ap, str);
	vsprintf(argBuf, str, ap);
	va_end(ap);

	printf( "%s\n", argBuf);
    }

}
