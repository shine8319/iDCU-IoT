#include <stdio.h> 
#include <string.h> 
#include <unistd.h>
#include <sys/ipc.h>
#include <sys/shm.h>


#include "../include/expat.h" 
#include "../include/PointManager.h"


char deviceparser_Buff[BUFFSIZE]; 

int deviceparser_Depth = 0; 
int deviceparser_is_value = 0; 

const char *current_el;   // 가장 최근 태그 기억용 
const char **current_id;  // 태그의 특성값 기억용 

int  deviceparser_user_num = 0;   // 몇개의 레코드가 저장되었는지 검사  
/*
typedef struct DEVINFO {
	char processcycle[MAX_POINT];
	char getDeviceEa;
} DEVINFO;
*/
DEVINFO devinfo;


void deviceparser_parser(void *data, const char *s, int len) 
{ 
	int i = 0;  
	int index =  0;     
	char buf[1024] = {0x00, }; 

	int cycle = 0;
	int expire = 0;


	for (i = 0; i < len; i++) 
	{ 
		if (s[i] != '\n' && s[i] != '\t' && i < len) 
		{ 
			buf[index] = s[i];  
			index++; 
		} 
	} 

	if (deviceparser_is_value && current_el && (buf[0] != 0x00)) 
	{ 
		if (current_id[0]) 
		{
			//printf("==> %s : %s (%s)\n", current_el, current_id[1], buf); 
		}
		else 
		{
			//printf("==> %s (%s)\n", current_el, buf); 
			if (strcmp(current_el,"PROCESSCYCLE") == 0) 
			{
				cycle = atoi(buf);
				devinfo.processcycle[deviceparser_user_num-1] = cycle;
				printf("  cycle = %d\n", devinfo.processcycle[deviceparser_user_num-1]);
			}
			else if (strcmp(current_el,"EXPIRETIME") == 0) 
			{
				expire = atoi(buf);
				devinfo.expiretime[deviceparser_user_num-1] = expire;
				printf("  expire time = %d\n", devinfo.expiretime[deviceparser_user_num-1]);
			}


		}

	} 
} 

// xml (여는)태그를 만났을 때 호출되는 함수다. 
// depth를 1증가 시키고, depth가 0즉 처음 만나나는 테그라면  
// version을 검사한다. 물론 테그(el)를 직접 비교해도 될 것이다.        
void 
deviceparser_start(void *data, const char *el, const char **attr) { 
	deviceparser_is_value = 1; 
	current_el = el; 

	current_id = attr; 
	if(deviceparser_Depth == 0)  
	{ 
		printf("Version is %s\n\n", current_id[1]);  
	} 
	if (strcmp(current_el,"DEVINFO") == 0) 
	{ 
		printf("[%d] %s \n", deviceparser_user_num, current_el); 
		deviceparser_user_num++; 
	} 
	deviceparser_Depth++; 
} 

// xml (닫는)태그를 만났을 때 호출되는 함수다. 
// 닫히게 되므로 depth가 1감소할 것이다.  
	void 
deviceparser_end(void *data, const char *el)  
{ 
	deviceparser_is_value = 0; 
	deviceparser_Depth--; 
}  

DEVINFO deviceparser(const char *path) { 

	FILE *fp;
	// xml 파서를 생성한다. 
	XML_Parser p = XML_ParserCreate(NULL); 
	if (! p)  
	{ 
		fprintf(stderr, "Couldn't allocate memory for parser\n"); 
		exit(-1); 
	} 

	//fp = fopen("usn_node_info.xml","r");
	fp = fopen(path,"r");

	// xml의 element를 만났을 때 호출될 함수를 등록한다.  
	// start는 처음태그, end는 마지막 태그를 만났을 때 호출된다. 
	XML_SetElementHandler(p, deviceparser_start, deviceparser_end); 

	// 실제 데이터 (예 : <tag>data</tag>의 data)를 처리하기 위해서  
	// 호출될 함수를 등록한다. 
	XML_SetCharacterDataHandler(p, deviceparser_parser);     

	for (;;)  
	{ 
		int done; 
		int len; 

		len = fread(deviceparser_Buff, 1, BUFFSIZE, fp); 
		if (ferror(fp)) { 
			fprintf(stderr, "Read error\n"); 
			exit(-1); 
		} 
		done = feof(fp); 

		if (! XML_Parse(p, deviceparser_Buff, len, done)) { 
			fprintf(stderr, "Parse error at line %d:\n%s\n", 
					XML_GetCurrentLineNumber(p), 
					XML_ErrorString(XML_GetErrorCode(p))); 
			exit(-1); 
		} 
		if (done) 
			break; 
	} 
	printf("\n\nDevice is : %dea\n", deviceparser_user_num); 
	devinfo.getDeviceEa = deviceparser_user_num;

	XML_ParserFree(p);
	close(fp);

	return devinfo;

	} 
