
#include <stdio.h>
#include <stdlib.h>
#include <time.h>
#include <sys/shm.h>
#include <signal.h>

//#include "sf2.h"
#define DEBUG

#include "./include/sfsock.h"
#include "./include/expat.h" 
#include "./include/PointManager.h"
#include "./include/CommManager.h"

extern NODEINFO pointparser(const char *path);
extern COMMPARSING CommDriver( unsigned char *pack, int size );

int flag = 0;
serial_source src;
//int packets_read, packets_written, num_clients;
char* db_addr="127.0.0.1";
char* db_port="8668";
char* dl_addr="127.0.0.1";
char* dl_port="5000";

unsigned char user_quit = 0;

void quitsignal(int sig) 
{
	user_quit = 1;
}


unsigned char* convert(unsigned char* ch, int size) {
	int i, buf;
	unsigned char* result;
	result = (char*)malloc(size*2+4);
	// if size == 30, result[ ] is 64 bytes
	//makeresult[0] = 52;
	result[0] = 52;
	result[1] = 50;
	for(i=0; i<(size-1); i++) {
		buf = ch[i];
		//	printf("\n buf = %p",buf);
		//
		if((buf/16) < 10){
			result[(i*2)+2] = (buf/16 + 48);
			//printf(" A data ");

		} else {
			result[i*2+2] = (buf/16 + 87);
			//printf(" B data ");
		}

		if((buf%16) < 10) {
			result[i*2+3] = (buf%16 + 48);
			//printf(" C data ");
		} else {
			result[i*2+3] = (buf%16 + 87);
			//printf(" D data ");
		}

	}
	result[i*2+2]=10;
	return result;
}

void stderr_msg1(serial_source_msg problem) {
	fprintf(stderr, "Note: %s\n", msgs[problem]);
}

int verbose_set=0;
int nodeid;
int recv_cnt = 0;

int main(int argc, char **argv) {

	int i;
	NODEINFO tag;
	POINTINFO point[32];

	int byte;
	int prev_byte=0;
	int valid_start_packet=0;
	int byte_cnt=0;
	time_t pack_time;
	time_t start_time;
	unsigned char pack[128];
	int esc_i=0;

	int escape_cnt=0;
	COMMPARSING getData;

	int rtrn;
	sleep(4);

	if (argc < 4) {
		printf ("%d ",argc);
		fprintf(stderr, "Usage: %s <device> <rate> <verbose option>\n", argv[0]);
		fprintf(stderr, "example : %s /dev/ttyUSB0 57600 ID -v\n", argv[0]);
		exit(2);
	}

	if(argc>2)
	{
		// non blocking
		//src = open_serial_source(argv[1], atoi(argv[2]), 1, stderr_msg1);

		// blocking
		src = open_serial_source(argv[1], atoi(argv[2]), 0, stderr_msg1);
		nodeid = atoi(argv[3]);

	}
	//printf("%x\n", nodeid );
	if(argc==5){
		if(argv[4][0]=='-' && argv[4][1]=='v') {
			printf("verbose mode starting\n");
			verbose_set = 1;
		}
	}

	if (!src){
		fprintf(stderr, "port Open Error.. %s:%s\n", argv[1], argv[2]);
		exit(1);
	}

	(void)signal(SIGINT, quitsignal);
	printf("Press <ctrl+c> to user_quit. \n");

	tag = pointparser("usn_node_info.xml");

	printf("Received size = %d\n", sizeof(tag));
	for( i = 0; i < tag.getPointSize; i++)
	{
		printf("tag.id[%d] = %d\n", i,tag.id[i]);
		printf("tag.type[%d] = %d\n", i,tag.type[i]);
		printf("tag.addr[%d] = %d\n", i,tag.addr[i]);
		printf("tag.size[%d] = %d\n", i,tag.size[i]);
	}

	//POINTINFO point[tag.getPointSize];
	printf("8 * %d(node) = %d\n", tag.getPointSize, sizeof(point));

	for( i = 0; i < tag.getPointSize; i++)
	{

		if ( -1 == ( point[i].id = shmget( (key_t)tag.id[i], tag.size[i], IPC_CREAT|0666)))
		{
			printf( "shared memory create error\n");
			return -1;
		}

		if ( ( void *)-1 == ( point[i].addr = shmat( point[i].id, ( void *)0, 0)))
		{
			printf( "shared memory attch error\n");
			return -1;
		}

	}




	time(&start_time);
	printf("time info will be provided\n");

	while(!user_quit){
		//printf("+++++++++++++++++++++++++++++++\n");
		byte = read_byte(src);

		if (byte > -1) 
		{
			if (byte == 0x45 && prev_byte == 0x7e ) 
			{
				valid_start_packet=1;
				pack[byte_cnt++] = byte;
			}
			else if ((byte == 0x7e) && (prev_byte == 0x7e) ) 
			{

					//printf("Nodeid =====) %d \n", nodeid);
					//printf("Nodeid ====-> %d \n", pack[14]);
				//if (verbose_set == 1  && nodeid == pack[14])
				if (verbose_set == 1  && 0 != pack[11] )
				{
					// simple escaping received packet
					for (esc_i=0;esc_i<100;esc_i++)
					{
						if (pack[esc_i] == 0x7d) 
						{
							pack[esc_i+1] = 0x20 ^ pack[esc_i+1];
							memcpy (pack+esc_i,pack+esc_i+1,100-esc_i-1);
							escape_cnt++;
						}
					}
					printf("\n");
					printf("\n");
					//  debug
					/*
					for(i = 0; i < byte_cnt-1-escape_cnt; i++)
					{
						printf("%02X ", pack[i]);
					}
					printf("\n");
					*/
	
					printf("Packet Size = %d, NodeID=0x%X ( %d )\n"
							, byte_cnt
							, pack[11]
							, pack[11] );

					getData = CommDriver( pack, byte_cnt-1-escape_cnt );
					for(i = 0; i < tag.getPointSize; i++)
					{
						if( tag.id[i] == getData.nodeid )
						{
							printf("Mach NodeID = %d\n", tag.id[i]);
							sprintf( (char *)point[i].addr, "%d;%d;%d;%d;%d;"
									//, getData.nodeid
									, getData.temp 
									, getData.humi
									, getData.lux
									, getData.bat
									, pack[12]
								   );
						}
					}

					for(i = 0; i < byte_cnt-1-escape_cnt; i++)
					{
						printf("%02X ", pack[i]);
					}
					printf("\n");
					escape_cnt = 0;

				}  // if (verbose_set == 1  && nodeid == pack[14])
				time(&pack_time);
				//printf("time  = %d \n",pack_time-start_time);
				//printf("\n");
				valid_start_packet = 0;
				byte_cnt = 0;

			}   //else if ((byte == 0x7e) && (prev_byte == 0x7e) ) 
			else if (verbose_set && valid_start_packet)
			{
				pack[byte_cnt] = byte;
				byte_cnt++;
			}

			if( byte_cnt > 127 )
			{
				printf("byte_cnt = %d\n", byte_cnt );
				byte_cnt = 0;
				valid_start_packet = 0;
			}

			printf("%02X ", byte);
			prev_byte=byte;
		}
	}




	
	// release shared memory
	for( i = 0; i < tag.getPointSize; i++ )
	{
		rtrn = detachSharedMem( 1, tag.id[i], point[i].addr, point[i].id );
		printf("rtrn = %d\n", rtrn);

	}


	printf("[CommManager] end of main loop \n");
	return 0;
}

void make_connect_sock(char* addr, char* port){
	setaddr(addr, port);
	flag = 1;
	sockconn();
}

void* deluge_socket_process(){
	m_deluge_process(src);
}

/*
void* sendingProcess(void* arg) {

	unsigned char *result;
	//Thread ### : Thread1
	//in : serial / out : DB, RFfiltering	
	int len, i, n_pck=1;

	for (;;)
	{
		printf("waiting for serial packet \n");
		unsigned char *packet = read_serial_packet(src, &len);
		//unsigned char *packet = NULL;
		//printf("serial packet length= %d",len);

		if (packet!=NULL) {
			//result = convert(packet, len+1);
			//socksend(result, len*2+3);

			for(i=0;i<len;i++) printf("%02x", packet[i]); printf("\n");
			free((void *)packet);
			n_pck=1;

			free((void *)packet);
			n_pck=2;
		}
	}
}
*/

