#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <fcntl.h>
#include <unistd.h>

#define		FIFO_FILE   "/tmp/fifo"
#define		BUFF_SIZE	1024

int main( void)
{
   int   fd;
   char *str   = "forum.falinux.com";
   char  buff[BUFF_SIZE];

   if ( -1 == ( fd = open( FIFO_FILE, O_WRONLY)))
   {
      perror( "open() 실행에러");
      exit( 1);
   }

   while(1)
   {
	   write( fd, str, strlen( str));
	   printf( "%s\n",  buff);
	   sleep(1);
   }
   close( fd);
}
