
#define BUFFSIZE	4096	
#define MAX_POINT	64	
#define MAX_DEVICE	32	


typedef struct NODEINFO {
	char id[MAX_POINT];
	char type[MAX_POINT];
	char addr[MAX_POINT];
	char size[MAX_POINT];
	char getPointSize;
} NODEINFO;

typedef struct DEVINFO {
	int processcycle[MAX_POINT];
	int expiretime[MAX_POINT];
	char getDeviceEa;
} DEVINFO;


/*
typedef struct POINTINFO {
	unsigned char id;
	unsigned char addr;
} POINTINFO;
*/
typedef struct POINTINFO {
	const void *id;
	const void *addr;
} POINTINFO;

	
