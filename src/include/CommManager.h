//#define USN			"/dev/ttyAMA0"
#define USN			"/dev/ttyUSB0"
//#define	BAUDRATE	57600	
#define	BAUDRATE	115200	
#define BUFFER_SIZE 1024 
#define	M2M_COMMAND	0
#define	USN_RETURN	1
#define	DI_PORT		6

enum {
	REBOOTNODE		= 0x01,
	GETNODELIST		= 0x03,
	DELETENODE		= 0x04,
	CHANGENODEINFO		= 0x12,
	GETCNT			= 0x13,
};


/*
typedef struct { 
long data_type; 
int data_num; 
unsigned char data_buff[BUFFER_SIZE]; 
} t_data; 
*/

typedef struct { 
long data_type; 
unsigned char nodeid; 
unsigned char group; 
unsigned char seq; 
unsigned char wo[DI_PORT]; 
unsigned int cnt[DI_PORT]; 
} t_node; 

typedef struct {
		unsigned char type;
		unsigned char length;
		unsigned char value[16];
} nodeCMDTLV;
		


int OpenSerial(void);
int closeSerial(int fd);
int ParsingReceiveValue(unsigned char* cvalue, int len, unsigned char* remainder, int remainSize );
int selectTag(unsigned char* buffer, int len );


enum {
	NODEID 	= 7,
	GROUP	= 4,
	SEQ		= 10,
	WO		= 13,
	CNT		= 31
};
