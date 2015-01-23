#include <stdio.h> 
#include <stdlib.h> 
#include <string.h> 
#include <unistd.h>
#include <sys/ipc.h>
#include <sys/msg.h>
#include <sys/shm.h>
#include <signal.h>
#include <sys/socket.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <arpa/inet.h>
#include <fcntl.h>
#include <errno.h>
#include <time.h>
#include <termio.h>
#include <sys/wait.h>

#include "../include/iDCU.h"
#define BUFFER_SIZE 1024

static int connect_nonb(int sockfd, const struct sockaddr *saptr, int salen, int nsec);

INT32 TCPServer(INT32 port) {

    INT32 server_sock; 
    int client_sock; 

    struct sockaddr_in servaddr; //server addr
    struct sockaddr_in clientaddr; //client addr
    int client_addr_size;
    INT32 bufsize = 100000;
    INT32 rn = sizeof(INT32);
    INT32 flags;

    printf("Start Socket Server!!\n");

    /* create socket */
    server_sock = socket(PF_INET, SOCK_STREAM, 0);

    // add by hyungoo.kang
    // solution ( bind error : Address already in use )
    if( setsockopt( server_sock, SOL_SOCKET, SO_REUSEADDR, (void*)&bufsize, (socklen_t)rn) < 0 ) {
	perror("Error setsockopt()");
	return -1;
    }

    memset(&servaddr, 0, sizeof(servaddr));
    servaddr.sin_family = AF_INET;
    servaddr.sin_addr.s_addr = htonl(INADDR_ANY);
    servaddr.sin_port = htons(port);

    if (bind(server_sock, (struct sockaddr*)&servaddr, sizeof(servaddr)) < 0) {
	printf("server socket bind error\n");
	return -1;
    }

    listen(server_sock, 5);

    client_addr_size = sizeof(clientaddr);
    client_sock = accept(server_sock, (struct sockaddr*)&clientaddr, &client_addr_size);


    if((flags = fcntl(client_sock, F_GETFL, 0)) == -1 ) {
	perror("fcntl");
	return;
    }
    if(fcntl(client_sock, F_GETFL, flags | O_NONBLOCK) == -1) {
	perror("fcntl");
	return;
    }

    close( server_sock );

    printf("New Connection, Client IP : %s (%d)\n",	inet_ntoa(clientaddr.sin_addr), client_sock);
    //close( client_sock );

    return client_sock;

}



INT32 TCPClient(INT8 *serverIP, UINT16 port )
{
    INT32 server_sock;
    struct sockaddr_in servaddr; //server addr
    INT32 rtrn;
    INT32 flags;

    //printf("IP %s PORT %d\n", serverIP, port);
    server_sock = socket(PF_INET, SOCK_STREAM, 0);
    if( server_sock < 0 )
    {
	//perror("error : ");
	return -1;	
    }

    memset(&servaddr, 0, sizeof(servaddr));
    servaddr.sin_family = AF_INET;
    servaddr.sin_addr.s_addr = inet_addr(serverIP);
    //servaddr.sin_port = htons(atoi(port));
    servaddr.sin_port = htons((port));

    if(connect_nonb(server_sock, (struct sockaddr *)&servaddr, sizeof(servaddr),10) != 0)
    {
	close( server_sock );
	return -2;
    }


	 /*
    rtrn = connect(server_sock, (struct sockaddr *)&servaddr, sizeof(servaddr));
    if( rtrn == -1 )
    {
	close( server_sock );
	return -2;
    }
    */

    /*
       if(ConnectWait(server_sock, (struct sockaddr *)&servaddr, sizeof(servaddr), 5) < 0) 
       { 
       perror("error"); 
       return -1;
       } 
       else 
       { 
       printf("Connect Success\n"); 
       } 
     */

    /*
    if((flags = fcntl(server_sock, F_GETFL, 0)) == -1 ) {
	close( server_sock );
	//perror("fcntl");
	return -3;
    }
    if(fcntl(server_sock, F_GETFL, flags | O_NONBLOCK) == -1) {
	close( server_sock );
	//perror("fcntl");
	return -4;
    }
    */

    return server_sock;

}

static int connect_nonb(int sockfd, const struct sockaddr *saptr, int salen, int nsec)
{
    int             flags, n, error;
    socklen_t       len;
    fd_set          rset, wset;
    struct timeval  tval;

    flags = fcntl(sockfd, F_GETFL, 0);
    fcntl(sockfd, F_SETFL, flags | O_NONBLOCK);

    error = 0;
    if ( (n = connect(sockfd, (struct sockaddr *) saptr, salen)) < 0)
	if (errno != EINPROGRESS)
	    return(-1);

    /* Do whatever we want while the connect is taking place. */

    if (n == 0)
	goto done;  /* connect completed immediately */

    FD_ZERO(&rset);
    FD_SET(sockfd, &rset);
    wset = rset;
    tval.tv_sec = nsec;
    tval.tv_usec = 0;

    if ( (n = select(sockfd+1, &rset, &wset, NULL,
		    nsec ? &tval : NULL)) == 0) {
	close(sockfd);      /* timeout */
	errno = ETIMEDOUT;
	return(-1);
    }

    if (FD_ISSET(sockfd, &rset) || FD_ISSET(sockfd, &wset)) {
	len = sizeof(error);
	if (getsockopt(sockfd, SOL_SOCKET, SO_ERROR, &error, &len) < 0)
	    return(-1);         /* Solaris pending error */
    } else
	printf("select error: sockfd not set\n");

done:
    fcntl(sockfd, F_SETFL, flags);  /* restore file status flags */
    if (error) {
	close(sockfd);      /* just in case */
	errno = error;
	return(-1);
    }
    return(0);
}

int ConnectWait(int sockfd, struct sockaddr *saddr, int addrsize, int sec) 
{ 
    int newSockStat; 
    int orgSockStat; 
    int res, n; 
    fd_set  rset, wset; 
    struct timeval tval; 

    int error = 0; 
    int esize; 

    if ( (newSockStat = fcntl(sockfd, F_GETFL, NULL)) < 0 )  
    { 
	perror("F_GETFL error"); 
	return -1; 
    } 

    orgSockStat = newSockStat; 
    newSockStat |= O_NONBLOCK; 

    // Non blocking 상태로 만든다.  
    if(fcntl(sockfd, F_SETFL, newSockStat) < 0) 
    { 
	perror("F_SETLF error"); 
	return -1; 
    } 

    // 연결을 기다린다. 
    // Non blocking 상태이므로 바로 리턴한다. 
    if((res = connect(sockfd, saddr, addrsize)) < 0) 
    { 
	printf("RES : %d\n", res); 
	if (errno != EINPROGRESS) 
	    return -1; 
    } 
    else
	printf("RES : %d\n", res); 

    return 0;

    /*
    // 즉시 연결이 성공했을 경우 소켓을 원래 상태로 되돌리고 리턴한다.  
    if (res == 0) 
    { 
    printf("Connect Success\n"); 
    fcntl(sockfd, F_SETFL, orgSockStat); 
    return 1; 
    } 

    FD_ZERO(&rset); 
    FD_SET(sockfd, &rset); 
    wset = rset; 

    tval.tv_sec        = sec;     
    tval.tv_usec    = 0; 

    if ( (n = select(sockfd+1, &rset, &wset, NULL, NULL)) == 0) 
    { 
    // timeout 
    errno = ETIMEDOUT;     
    return -1; 
    } 

    // 읽거나 쓴 데이터가 있는지 검사한다.  
    if (FD_ISSET(sockfd, &rset) || FD_ISSET(sockfd, &wset) ) 
    { 
    printf("Read data\n"); 
    esize = sizeof(int); 
    if ((n = getsockopt(sockfd, SOL_SOCKET, SO_ERROR, &error, (socklen_t *)&esize)) < 0) 
    return -1; 
    } 
    else 
    { 
    perror("Socket Not Set"); 
    return -1; 
    } 


    fcntl(sockfd, F_SETFL, orgSockStat); 
    if(error) 
    { 
    errno = error; 
    perror("Socket"); 
    return -1; 
    } 
     */

    //return 1; 
}
int TCPParsingReceiveValue(unsigned char* cvalue, int len, unsigned char* remainder, int remainSize )
{


    unsigned char setBuffer[BUFFER_SIZE];
    int i;
    int offset;
    char* token ;
    UINT8 parsing[128][16];
    UINT8 setString[256];
    UINT8 parsingCnt = 1;

    /*
       for( i = 0; i < len; i++ )
       {

       if( len == 237 && cvalue[i] == '"' && cvalue[i+1] == ':' && cvalue[i+2] == '"')
       {

       token = strtok( cvalue, ",");
       strcpy( parsing[parsingCnt++], token );
    //printf("[%d] %s\n", parsingCnt-1, parsing[parsingCnt-1] ) ;

    while( token = strtok( NULL, "," ) ) 
    {

    strcpy( parsing[parsingCnt++], token );
    //printf("[%d] %s\n", parsingCnt-1, parsing[parsingCnt-1] ) ;
    }

    sprintf(setString, "101:%s 102:%s 103:%s 104:%s 105:%s 106:%s 107:%s 108:%s 111:%s 112:%s 113:%s 114:%s 115:%s 116:%s 117:%s 118:%s"
    , parsing[10], parsing[11], parsing[13], parsing[12], parsing[29]
    , parsing[30], parsing[31], parsing[32], parsing[14], parsing[15]
    , parsing[16], parsing[28], parsing[17], parsing[18], parsing[20]
    , parsing[21]);

    //writeLog( "/work/test", setString);
    printf("%s\n", setString);

    selectTag( setString, strlen(setString) );

    return 0;

    }

    }
     */

    remainSize = len - remainSize;
    memcpy( remainder, cvalue+(len-(remainSize)), remainSize );


    return remainSize;
}

int sender( int *tcp, UINT8 *packet, UINT32 size )
{
    int rtrn;
    int i;
    /*
       int reTry = 3;

       do {
       rtrn = send(*tcp, packet,  size, MSG_NOSIGNAL);
       printf("Send rtrn %d\n", rtrn);
       if( rtrn < 0 )
       {
       sleep(1);
       reTry--;
       }
       else
       reTry = 0;

       } while( reTry );
     */
    rtrn = send(*tcp, packet,  size, MSG_NOSIGNAL);

    for( i = 0; i < size; i++ )
	printf("%02X ", packet[i] );
    printf("\n");
    printf("to M/W Send size %d\n", rtrn);
    if( rtrn != -1 )
    {
	rtrn = receive(tcp, 10);
	if( rtrn == 0 || rtrn == 2 )
	{
	    return 0;
	}
	else
	{
	    return -1;
	    // error
	}
    }

    return -1;
}

int receive( int *tcp, int timeout ) {

    fd_set control_msg_readset;
    struct timeval control_msg_tv;
    unsigned char DataBuf[BUFFER_SIZE];
    int ReadMsgSize;

    unsigned char receiveBuffer[BUFFER_SIZE];
    int receiveSize = 0;
    unsigned char remainder[BUFFER_SIZE];
    int parsingSize = 0;


    int rtrn = 0;
    int i;
    int nd;

    FD_ZERO(&control_msg_readset);
    //printf("Receive Ready!!!\n");

    //while( 1 ) 
    {

	FD_SET(*tcp, &control_msg_readset);
	control_msg_tv.tv_sec = timeout;
	//control_msg_tv.tv_usec = 10000;
	control_msg_tv.tv_usec = 0;	// timeout check 5 second

	nd = select( *tcp+1, &control_msg_readset, NULL, NULL, &control_msg_tv );		
	if( nd > 0 ) 
	{

	    memset( DataBuf, 0, sizeof(DataBuf) );
	    ReadMsgSize = recv( *tcp, &DataBuf, BUFFER_SIZE, MSG_DONTWAIT);
	    if( ReadMsgSize > 0 ) 
	    {
		for( i = 0; i < ReadMsgSize; i++ ) {
		    printf("%-3.2x", DataBuf[i]);
		}
		printf("\n");
		printf("recv data size : %d\n", ReadMsgSize);

		//if( ReadMsgSize >= BUFFER_SIZE )
		//continue;

		memcpy( receiveBuffer+receiveSize, DataBuf, ReadMsgSize );
		receiveSize += ReadMsgSize;

		parsingSize = TCPParsingReceiveValue(receiveBuffer, receiveSize, remainder, parsingSize);
		memset( receiveBuffer, 0 , sizeof(BUFFER_SIZE) );
		receiveSize = 0;
		memcpy( receiveBuffer, remainder, parsingSize );
		receiveSize = parsingSize;

		memset( remainder, 0 , sizeof(BUFFER_SIZE) );

	    } 
	    else {
		sleep(1);
		printf("receive None\n");
		//break;
	    }
	    ReadMsgSize = 0;

	} 
	else if( nd == 0 ) 
	{
	    printf("timeout\n");
	    //shutdown( *tcp, SHUT_WR );
	    rtrn = -1;
	    //break;
	}
	else if( nd == -1 ) 
	{
	    printf("error...................\n");
	    //shutdown( *tcp, SHUT_WR );
	    rtrn = -1;
	    //break;
	}
	nd = 0;

    }	// end of while

    //printf("Disconnection client....\n");

    //close( *tcp);
    return rtrn;

}


