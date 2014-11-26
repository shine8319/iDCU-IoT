#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <arpa/inet.h>
#include <sys/types.h>
#include <sys/socket.h>

//#define  BUFF_SIZE   1024

int UDPConnect( int port )
{
   int   sock;
   //int   client_addr_size;
   int   rtrn;
   int broadcastEnable = 1;

   struct sockaddr_in   server_addr;
   //struct sockaddr_in   client_addr;

   //char   buff_rcv[BUFF_SIZE+5];
   //char   buff_snd[BUFF_SIZE+5];


   //client_addr_size  = sizeof( client_addr);
   sock  = socket( AF_INET, SOCK_DGRAM, 0);

   if( -1 == sock)
   {
      printf( "socket error\n");
      exit( 1);
   }

   rtrn = setsockopt(sock, SOL_SOCKET, SO_BROADCAST, &broadcastEnable, sizeof(broadcastEnable));

   memset( &server_addr, 0, sizeof( server_addr));

   server_addr.sin_family     = AF_INET;
   server_addr.sin_port       = htons( port );
   //server_addr.sin_addr.s_addr= htonl( INADDR_ANY);
   server_addr.sin_addr.s_addr= htonl( INADDR_BROADCAST );

   if( -1 == bind( sock, (struct sockaddr*)&server_addr, sizeof( server_addr) ) )
   {
      printf( "bind() error\n");
      exit( 1);
   }

   /*
   while( 1)
   {
      memset( buff_rcv, 0, BUFF_SIZE+5 );
      memset( buff_snd, 0, BUFF_SIZE+5 );

      rtrn = recvfrom( sock, buff_rcv, BUFF_SIZE, 0 , ( struct sockaddr*)&client_addr, &client_addr_size);
      printf("from %s(%d) receive: %s\n", inet_ntoa(client_addr.sin_addr), ntohs(client_addr.sin_port), buff_rcv);

      sprintf( buff_snd, "return Msg : %s", buff_rcv);
      rtrn = sendto( sock, buff_snd, strlen( buff_snd)+1, 0,( struct sockaddr*)&client_addr, sizeof( client_addr));
printf("return %d\n", rtrn);


   }
*/
   return sock;
}
