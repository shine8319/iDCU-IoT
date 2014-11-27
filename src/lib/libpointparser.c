#include <stdio.h> 
#include <string.h> 
#include <unistd.h>
#include <sys/ipc.h>
#include <sys/shm.h>

#include "../include/expat.h" 
#include "../include/iDCU.h"
//#include "../include/PointManager.h"


#define BUFFSIZE	4096	

char Buff[BUFFSIZE]; 

int Depth = 0; 
int is_value = 0; 

const char *current_el;   // 가장 최근 태그 기억용 
const char **current_id;  // 태그의 특성값 기억용 

int  user_num = 0;   // 몇개의 레코드가 저장되었는지 검사  
NODEINFO taginfo;


void parser(void *data, const char *s, int len) 
{ 
	int i = 0;  
	int index =  0;     
	char buf[1024] = {0x00, }; 

	int id = 0;
	int type = 0;
	int addr = 0;
	int size = 0;


	for (i = 0; i < len; i++) 
	{ 
		if (s[i] != '\n' && s[i] != '\t' && i < len) 
		{ 
			buf[index] = s[i];  
			index++; 
		} 
	} 

	if (is_value && current_el && (buf[0] != 0x00)) 
	{ 
		if (current_id[0]) 
		{
			//printf("==> %s : %s (%s)\n", current_el, current_id[1], buf); 
		}
		else 
		{
			//printf("==> %s (%s)\n", current_el, buf); 
		    /*
			if (strcmp(current_el,"NODEID") == 0) 
			{
				id = atoi(buf);
				taginfo.id[user_num-1] = id;
				printf("  ID = %d\n", taginfo.id[user_num-1]);
			}
			else if (strcmp(current_el,"SENSORTYPE") == 0) 
			{
				type = atoi(buf);
				taginfo.type[user_num-1] = type;
				printf("TYPE = %d\n", taginfo.type[user_num-1]);
			}
			else if (strcmp(current_el,"DATAADDR") == 0) 
			{
				addr = atoi(buf);
				taginfo.addr[user_num-1] = addr;
				printf("ADDR = %d\n", taginfo.addr[user_num-1]);
			}
			else if (strcmp(current_el,"DATASIZE") == 0) 
			{
				size = atoi(buf);
				taginfo.size[user_num-1] = size;
				printf("SIZE = %d\n", taginfo.size[user_num-1]);
			}
			*/
			if (strcmp(current_el,"IP") == 0) 
			{
				sprintf(taginfo.tag[user_num-1].ip, "%s",buf);
				printf("IP = %s\n", taginfo.tag[user_num-1].ip);
			}
			else if (strcmp(current_el,"PORT") == 0) 
			{
				sprintf(taginfo.tag[user_num-1].port, "%s",buf);
				printf("PORT = %s\n", taginfo.tag[user_num-1].port);
			}
			else if (strcmp(current_el,"DRIVER") == 0) 
			{
				sprintf(taginfo.tag[user_num-1].driver, "%s",buf);
				printf("DRIVER = %s\n", taginfo.tag[user_num-1].driver);
			}
			else if (strcmp(current_el,"BASESCANRATE") == 0) 
			{
				sprintf(taginfo.tag[user_num-1].basescanrate, "%s",buf);
				printf("BASESCANRATE = %s\n", taginfo.tag[user_num-1].basescanrate);
			}



		}

	} 
} 

// xml (여는)태그를 만났을 때 호출되는 함수다. 
// depth를 1증가 시키고, depth가 0즉 처음 만나나는 테그라면  
// version을 검사한다. 물론 테그(el)를 직접 비교해도 될 것이다.        
void 
start(void *data, const char *el, const char **attr) { 
	is_value = 1; 
	current_el = el; 

	current_id = attr; 
	if(Depth == 0)  
	{ 
		printf("Version is %s\n\n", current_id[1]);  
	} 
	if (strcmp(current_el,"NODEINFO") == 0) 
	{ 
		printf("[%d] %s \n", user_num, current_el); 
		user_num++; 
	} 
	Depth++; 
} 

// xml (닫는)태그를 만났을 때 호출되는 함수다. 
// 닫히게 되므로 depth가 1감소할 것이다.  
	void 
end(void *data, const char *el)  
{ 
	is_value = 0; 
	Depth--; 
}  

NODEINFO pointparser(const char *path) { 

	FILE *fp;
	// xml 파서를 생성한다. 
	XML_Parser p = XML_ParserCreate(NULL); 
	//printf("XML_Parser p = XML_ParserCreate(NULL); \n");
	if (! p)  
	{ 
		fprintf(stderr, "Couldn't allocate memory for parser\n"); 
		exit(-1); 
	} 

	//fp = fopen("usn_node_info.xml","r");
	fp = fopen(path,"r");
	//printf("fp = fopen(path,r);\n");

	// xml의 element를 만났을 때 호출될 함수를 등록한다.  
	// start는 처음태그, end는 마지막 태그를 만났을 때 호출된다. 
	//printf("XML_SetElementHandler(p, start, end);\n");
	XML_SetElementHandler(p, start, end); 

	// 실제 데이터 (예 : <tag>data</tag>의 data)를 처리하기 위해서  
	// 호출될 함수를 등록한다. 
	//printf("XML_SetCharacterDataHandler(p, parser); \n");
	XML_SetCharacterDataHandler(p, parser);     

	for (;;)  
	{ 

		int done; 
		int len; 

		//printf("for (;;) \n");
		len = fread(Buff, 1, BUFFSIZE, fp); 
		if (ferror(fp)) { 
			fprintf(stderr, "Read error\n"); 
			exit(-1); 
		} 
		done = feof(fp); 

		if (! XML_Parse(p, Buff, len, done)) { 
			//fprintf(stderr, "Parse error at line %d:\n%s\n", 
			fprintf(stderr, "Parse error at line %ld:\n%s\n", 
					XML_GetCurrentLineNumber(p), 
					XML_ErrorString(XML_GetErrorCode(p))); 
			exit(-1); 
		} 
		if (done) 
			break; 
	} 
	printf("\n\nNode is : %dea\n", user_num); 
	taginfo.getPointSize = user_num;

	XML_ParserFree(p);
	close((int)fp);

	return taginfo;

	} 
