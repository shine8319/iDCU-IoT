#include <stdio.h> 
#include <string.h> 
#include <unistd.h>
#include <sys/ipc.h>
#include <sys/shm.h>

#include "../../include/expat.h" 
#include "../../include/iDCU.h"


#define BUFFSIZE	4096	

static char Buff[BUFFSIZE]; 

static int Depth = 0; 
static int is_value = 0; 

static const char *current_el;   // 가장 최근 태그 기억용 
static const char **current_id;  // 태그의 특성값 기억용 

static int  user_num = 0;   // 몇개의 레코드가 저장되었는지 검사  
static SERIALINFO taginfo;


static void parser(void *data, const char *s, int len) 
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

	    if (strcmp(current_el,"BAUD") == 0) 
	    {
		sprintf(taginfo.comport.baud, "%s",buf);
		printf("%s\n", taginfo.comport.baud);
	    }
	    else if (strcmp(current_el,"PARITY") == 0) 
	    {
		sprintf(taginfo.comport.parity, "%s",buf);
		printf("%s\n", taginfo.comport.parity);
	    }
	    else if (strcmp(current_el,"DATABIT") == 0) 
	    {
		sprintf(taginfo.comport.databit, "%s",buf);
		printf("%s\n", taginfo.comport.databit);
	    }
	    else if (strcmp(current_el,"STOPBIT") == 0) 
	    {
		sprintf(taginfo.comport.stopbit, "%s",buf);
		printf("%s\n", taginfo.comport.stopbit);
	    }
	    else if (strcmp(current_el,"FLOWCONTROL") == 0) 
	    {
		sprintf(taginfo.comport.flowcontrol, "%s",buf);
		printf("%s\n", taginfo.comport.flowcontrol);
	    }
	    else if (strcmp(current_el,"MODE") == 0) 
	    {
		sprintf(taginfo.connect.mode, "%s",buf);
		printf("%s\n", taginfo.connect.mode);
	    }
	    else if (strcmp(current_el,"IP") == 0) 
	    {
		sprintf(taginfo.connect.ip, "%s",buf);
		printf("%s\n", taginfo.connect.ip);
	    }
	    else if (strcmp(current_el,"PORT") == 0) 
	    {
		sprintf(taginfo.connect.port, "%s",buf);
		printf("%s\n", taginfo.connect.port);
	    }
	    else if (strcmp(current_el,"LOCALPORT") == 0) 
	    {
		sprintf(taginfo.connect.localport, "%s",buf);
		printf("%s\n", taginfo.connect.localport);
	    }
	    else if (strcmp(current_el,"TIMEOUT") == 0) 
	    {
		sprintf(taginfo.connect.timeout, "%s",buf);
		printf("%s\n", taginfo.connect.timeout);
	    }

	}

    } 
} 

// xml (여는)태그를 만났을 때 호출되는 함수다. 
// depth를 1증가 시키고, depth가 0즉 처음 만나나는 테그라면  
// version을 검사한다. 물론 테그(el)를 직접 비교해도 될 것이다.        
static void 
start(void *data, const char *el, const char **attr) { 
    is_value = 1; 
    current_el = el; 

    current_id = attr; 
    if(Depth == 0)  
    { 
	//printf("Version is %s\n\n", current_id[1]);  
    } 
    //if (strcmp(current_el,"NODEINFO") == 0) 
    //{ 
	//printf("[%d] %s \n", user_num, current_el); 
	//user_num++; 
    //} 
    Depth++; 
} 

// xml (닫는)태그를 만났을 때 호출되는 함수다. 
// 닫히게 되므로 depth가 1감소할 것이다.  
    static 	void 
end(void *data, const char *el)  
{ 
    is_value = 0; 
    Depth--; 
}  

SERIALINFO serialInfoParser(const char *path) { 

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
    printf("\n\nTag is : %dea\n", user_num); 
    //taginfo.getPointSize = user_num;

    XML_ParserFree(p);
    close((int)fp);

    return taginfo;

} 
