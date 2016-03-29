#define MAX_POINT	64	
#define MAX_DEVICE	32	
#define MAX_DRIVER	64
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

// add serial tab 2015.05.15
typedef struct COMPORTINFO_VAR  {
	UINT8 baud;
	UINT8 parity;
	UINT8 databit;
	UINT8 stopbit;
	UINT8 flowcontrol;

} __attribute__ ((__packed__)) COMPORTINFO_VAR;

typedef struct CONNECTINFO_VAR {
	UINT8 mode;
	UINT32 ip;
	UINT32 port;
	UINT32 localport;
	UINT32 timeout;
} __attribute__ ((__packed__)) CONNECTINFO_VAR;

typedef struct POINTINFO_VAR {
	UINT8 port;
	UINT32 value;
	UINT8 status;
} __attribute__ ((__packed__)) POINTINFO_VAR;

typedef struct TIMESYNCINFO_VAR {
	UINT32 address;
	UINT8 cycle;
} __attribute__ ((__packed__)) TIMESYNCINFO_VAR;

typedef struct VERSIONINFO_VAR {
	UINT8 mac[32];
	UINT8 ver[32];
} __attribute__ ((__packed__)) VERSIONINFO_VAR;

typedef struct CONFIGINFO_VAR {
	UINT32 setupport;
	UINT8 interval[8];
} __attribute__ ((__packed__)) CONFIGINFO_VAR;




typedef struct DEVINFO {
    struct lan_var	lan;
    struct wlan_var	wlan;

    UINT8   lanEnable;
    UINT8   wlanEnable;

    UINT8   lanAuto;
    UINT8   wlanAuto;


    struct security_var	security;

    COMPORTINFO_VAR comport;
    CONNECTINFO_VAR connect;

    POINTINFO_VAR point[8];

    TIMESYNCINFO_VAR timeSync;

    VERSIONINFO_VAR version;

    // 2015.08.12
    CONFIGINFO_VAR  config;

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
	unsigned char  data_buff[4096];
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


typedef struct COMPORTINFO {
	char baud[MAX_BUFFER];
	char parity[MAX_BUFFER];
	char databit[MAX_BUFFER];
	char stopbit[MAX_BUFFER];
	char flowcontrol[MAX_BUFFER];

} COMPORTINFO;

typedef struct CONNECTINFO {
	char mode[MAX_BUFFER];
	char ip[MAX_BUFFER];
	char port[MAX_BUFFER];
	char localport[MAX_BUFFER];
	char timeout[MAX_BUFFER];
} CONNECTINFO;

typedef struct SERIALINFO {
    COMPORTINFO comport;
    CONNECTINFO connect;
} SERIALINFO;


typedef struct INTERVALINFO {
	char interval[MAX_BUFFER];
} INTERVALINFO;

typedef struct CONFIGINFO {
	char debug[MAX_BUFFER];
	char setupport[MAX_BUFFER];
	INTERVALINFO port[8];
} CONFIGINFO;

typedef struct TIMESYNCINFO {
	char address[MAX_BUFFER];
	char cycle[MAX_BUFFER];
} TIMESYNCINFO;


// add 2016.03.22 for machine direct interface
typedef struct DRIVERTINFO {
	char name[MAX_BUFFER];
	char id[MAX_BUFFER];
	//char basescanrate[MAX_BUFFER];
	//char address[MAX_BUFFER];
	//char function[MAX_BUFFER];
	//char data[MAX_BUFFER];
	//char reserve1[MAX_BUFFER];
	//char reserve2[MAX_BUFFER];
	//char reserve3[MAX_BUFFER];
} DRIVERTINFO;

typedef struct DRIVERLIST {
	DRIVERTINFO driver[MAX_DRIVER];
	char getListCount;
} DRIVERLIST;

typedef struct DIRECTINTERFACEINFO{
	char driverId[MAX_BUFFER];
	char baseScanRate[MAX_BUFFER];
	char slaveId[MAX_BUFFER];
	char function[MAX_BUFFER];
	char address[MAX_BUFFER];
	char offset[MAX_BUFFER];
	char host[MAX_BUFFER];
	char port[MAX_BUFFER];
	char auth[MAX_BUFFER];
	char db[MAX_BUFFER];
	char key[MAX_BUFFER];
} DIRECTINTERFACEINFO;

typedef struct COMMAND_0X16_INFO {

    UINT8   driverId;
    UINT32   baseScanRate;
    UINT8   slaveId;
    UINT8   function;
    UINT16   address;
    UINT16   offset;

    UINT32   host;
    UINT16   port;
    UINT8   auth[16];
    UINT8   db;
    UINT8   key[32];

} __attribute__ ((__packed__)) COMMAND_0X16_INFO;


