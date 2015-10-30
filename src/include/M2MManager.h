#define  	BUFFER_SIZE   	1024
#define  	PACKET_SIZE		29	


enum {
	REBOOTNODE		= 0x01,
	TIMESYNC		= 0x02,
	CHANGENODEINFO	= 0x12,
	GETDATA			= 0x03,
	DELETENODE		= 0x04,
	GETNODELIST 	= 0x05,
};

/*
typedef struct { 
long data_type; 
int data_num; 
unsigned char data_buff[BUFFER_SIZE]; 
} t_data; 
*/


typedef struct Return_Selectdata {

	int size;
	char **past_result;

} Return_Selectdata;

typedef struct {
	char year;
	char month;
	char day;
	char hour;
	char min;
	char sec;
} __attribute__ ((__packed__)) AustemSendTimeOrigin;

typedef struct RTRNNODEGROUP{
	unsigned char nodeid;
	unsigned char group;
	AustemSendTimeOrigin time;
} RTRNNODEGROUP;

typedef struct QUERYTIME{
	char year;
	char month;
	char day;
	char hour;
	char minute;
	char sec;
} QUERYTIME;



typedef struct {
	char year;
	char month;
	char day;
	char hour;
	char min;
	char sec;
	unsigned char mSec[2];
	//short mSec;
} __attribute__ ((__packed__)) AustemSendTime;

typedef struct AustemSendData{
	unsigned char nodeid;
	unsigned char port;
	//unsigned int value;
	unsigned char value[4];
	AustemSendTime time;
} __attribute__ ((__packed__)) AustemSendData;

typedef struct AustemStatusSendData{
	unsigned char nodeid;
	unsigned char channelSize;
	//unsigned int channel[6];
	unsigned char channel[6][4];
	AustemSendTimeOrigin time;
} __attribute__ ((__packed__)) AustemStatusSendData;
/*
typedef struct NODEINFO {
	int id;
	int type;
	int addr;
	int size;
} NODEINFO;
*/
