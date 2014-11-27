#define MAX_POINT	64	
#define MAX_DEVICE	32	
#define MAX_BUFFER	32	
#define SQLITE_SAFE_FREE(x)	if(x){ x = NULL; }

typedef	    unsigned long   UINT32;
typedef	    unsigned short  UINT16;
typedef	    unsigned char   UINT8;
typedef	    long	    INT32;
typedef	    short	    INT16;
typedef	    char	    INT8;


enum {
	INSERT_BEGIN = 1,
	INSERT_END = 2,
};

typedef struct SAVEINFO {
    char*   lan_addr;
    char*   lan_netmask;
    char*   lan_gateway;
    char*   lan_dns;
} SAVEINFO;


struct security_var
{
    unsigned char ssid[32];
    unsigned char key_mgmt;
    unsigned char auth_alg;
    unsigned char group;
    unsigned char key_flag;
    unsigned char wep40[4][5];
    unsigned char wep104[4][13];
    unsigned char psk[32];
};

struct lan_var
{
    UINT32  lan_addr;
    UINT32  lan_netmask;
    UINT32  lan_gateway;
    UINT32  lan_dns;
};
struct wlan_var
{
    UINT32  lan_addr;
    UINT32  lan_netmask;
    UINT32  lan_gateway;
    UINT32  lan_dns;
};


typedef struct DEVINFO {
    struct lan_var	lan;
    struct wlan_var	wlan;
    struct security_var	security;

    UINT8   lanEnable;
    UINT8   wlanEnable;

} __attribute__ ((__packed__)) DEVINFO;


typedef struct { 
	UINT32 cnt[8];
} MGData; 

typedef struct { 
	UINT8 io[8];
} MGIOData; 



typedef struct {

	INT32 size;
	INT8 **data;

} SQLite3Data;

typedef struct {
	INT8 year;
	INT8 month;
	INT8 day;
	INT8 hour;
	INT8 min;
	INT8 sec;
} __attribute__ ((__packed__)) TimeOrigin;

typedef struct {
	UINT8 channel[8][4];
	//TimeOrigin time;
} __attribute__ ((__packed__)) StatusSendData;

typedef struct {
	long  data_type;
	int   data_num;
	//char  data_buff[BUFF_SIZE];
	unsigned char  data_buff[1024];
} t_data;

typedef struct READENV {
    UINT32 middleware_ip;
    UINT16 middleware_port;
    UINT8 manufacturer[20];
    UINT8 producno[20];
    UINT8 registration;
    UINT16 gatenode;
    UINT16 pan;
    UINT16 sensornode;
    UINT32 timeout;
} READENV;


// XML Parser
typedef struct TAGINFO {
	char id[MAX_BUFFER];
	char type[MAX_BUFFER];
	char addr[MAX_BUFFER];
	char size[MAX_BUFFER];
	char ip[MAX_BUFFER];
	char port[MAX_BUFFER];
	char driver[MAX_BUFFER];
	char basescanrate[MAX_BUFFER];
} TAGINFO;

typedef struct DEVICEINFO {
	char driver[MAX_BUFFER];
	char id[MAX_BUFFER];
} DEVICEINFO;


typedef struct NODEINFO {
	DEVICEINFO device[MAX_DEVICE];
	TAGINFO tag[MAX_DEVICE];
	char getPointSize;
} NODEINFO;


/*
typedef struct NODEINFO {
	char id[MAX_POINT];
	char type[MAX_POINT];
	char addr[MAX_POINT];
	char size[MAX_POINT];
	char getPointSize;
	char ip[MAX_POINT][32];
	char port[MAX_POINT][32];
	char driver[MAX_POINT][32];
	char basescanrate[MAX_POINT][32];
} NODEINFO;
*/


