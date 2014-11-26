#define SQLITE_SAFE_FREE(x)	if(x){ x = NULL; }

#define		DBPATH 			"/work/db/dabom-device"
#define  	BUFF_SIZE   	1024
#define		M2M_COMMAND		0
#define		USN_RETURN		1
#define		DI_PORT			6

/*
typedef struct {
	long  data_type;
	int   data_num;
	//char  data_buff[BUFF_SIZE];
	unsigned char  data_buff[BUFF_SIZE];
} t_data;
*/

typedef struct {
	unsigned char cnt[4]; 
} t_CntValue;


typedef struct {
	char 			wo[DI_PORT];
	char 			curWo[DI_PORT];
	unsigned int	cnt[DI_PORT];
	unsigned int	curCnt[DI_PORT];
	unsigned char	exist;
	//t_CntValue	cnt[DI_PORT];
	//t_CntValue	curCnt[DI_PORT];
} t_node;


typedef struct { 
	unsigned char nodeid; 
	unsigned char group; 
	unsigned char seq; 
	unsigned char wo[DI_PORT]; 
	unsigned int	cnt[DI_PORT];
	char 			changed[DI_PORT];
	char 			port;
	//t_CntValue	cnt[DI_PORT];
} t_getNode; 



typedef struct Return_RTDParser {
	char parsingdata[64];
	char alive;
	char nodeid;
	char *groupid;
	char *temp;
	char *humi;
	char *lux;
	char *bat;
	char *seq;
	signed char rtrn; 
} Return_RTDParser;

/*
enum {
	INSERT_BEGIN = 1,
	INSERT_END = 2,
};
*/


typedef struct Return_Selectdata {

	int size;
	char **past_result;

} Return_Selectdata;

typedef struct {
	unsigned char* nodeid; 
	int	nodeListSize;
} NODELIST;
/*
typedef struct NODEINFO {
	int id;
	int type;
	int addr;
	int size;
} NODEINFO;
*/

