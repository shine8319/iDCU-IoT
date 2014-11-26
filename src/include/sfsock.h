#include <stdio.h>
#include <unistd.h>
#include <string.h>
#include <signal.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <netdb.h>
#include "sf2.h"
#include "serialsource.h"


int sock;
int str_len;
struct sockaddr_in serv_addr;
char *buf;
int newsock;

extern serial_source src;


void sockconn();
void setaddr(char* addr, char* port);
void socksend(char* msg, int length);
void sockclose();
void* serverSock();
void* receviceProcess(void* socket);
void* sendingProcess(void* arg);
int readline(int fd, void *vptr, int maxlen);
void quit(int signum);
void* deluge_socket_process();
void make_connect_sock(char*,char*);

void* maxforserverSock();
void* maxforreceviceProcess(void* socket);
void* gatewaysendingProcess();

static char *msgs[] = {
	"unknown_packet_type",
	"ack_timeout"	,
	"sync"	,
	"too_long"	,
	"too_short"	,
	"bad_sync"	,
	"bad_crc"	,
	"closed"	,
	"no_memory"	,
	"unix_error"
};
