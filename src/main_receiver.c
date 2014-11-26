#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <fcntl.h>
#include <unistd.h>
#include <sys/shm.h>
#include <sys/shm.h>

#define  FIFO_FILE   "/tmp/fifo"
#define  BUFF_SIZE	2048 

int main( void)
{
   int   counter = 0;
   int   fd;
   char  test_buff[BUFF_SIZE];
   char  buff[BUFF_SIZE];

   if ( -1 == mkfifo( FIFO_FILE, 0666))
   {
      perror( "mkfifo() 실행에러");
      //exit( 1);
   }

   if ( -1 == ( fd = open( FIFO_FILE, O_RDWR)))
   {
      perror( "open() 실행에러");
      exit( 1);
   }

   while( 1 )
   {
      memset( buff, 0, BUFF_SIZE);
      read( fd, buff, BUFF_SIZE);
      printf( "%d: %s\n", counter++, buff);
   }
   close( fd);

   return 0;
}
