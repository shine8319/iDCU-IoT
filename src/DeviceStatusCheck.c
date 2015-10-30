#include <stdio.h> 
#include <dirent.h>
#include <stdlib.h> 
#include <string.h> 
#include <unistd.h>
#include <sys/ipc.h>
#include <sys/msg.h>
#include <sys/shm.h>
#include <signal.h>
#include <sys/socket.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <arpa/inet.h>
#include <fcntl.h>
#include <errno.h>
#include <time.h>
#include <termio.h>
#include <sys/wait.h>

#include "./include/wiringPi.h"
#include "./include/iDCU.h"
#include "./include/writeLog.h"
#include "./include/getDiffTime.h"

#include "./include/debugPrintf.h"
#include "./include/parser/configInfoParser.h"

#define NETWORKUSEINFOPATH  "/work/config/networkUse.config"
#define OPTIONSINFOPATH "/work/config/options.config"

#define CONFIGINFOPATH "/work/config/config.xml"
#define LOGPATH "/work/log/DeviceStatusCheck"
#define DBPATH "/work/db/comm"
#define SQLITE_SAFE_FREE(x)	if(x){ x = NULL; }
#define BUFFER_SIZE 2048
#define	BAUDRATE	115200	
#define DI_PORT		8
#define LED_PORT	2
#define BUTTON_PORT	3

enum {
    BLUE = 0,
    WHITE = 1
};



char diPin[DI_PORT] = { 5,6,4,14,13, 12, 3, 2 };	// c1 pin 18,22,16,23
char ledPin[LED_PORT] = { 10, 11 };
char buttonPin[BUTTON_PORT] = { 7, 1, 0 };
char doPin[4] = { 13, 12, 3, 2 };
/*
   char diPin[4] = { 5,6,4,14 };	// c1 pin 18,22,16,23
 */
/* 검사 주기(초) */
#define CHECK_SECOND        1  

/* 실행 서버 패스 및 이름 */
#define APP_PATH            "/work/smart"
#define APP1_NAME            "SetupManager.o"
#define APP2_NAME            "SerialToEthern"
#define APP3_NAME            "ModbusRTU.o"
#define LOGPATH "/work/log/DeviceStatusCheck"

int check_process(char* appName);
void get_timef(time_t, char *);
static void resetLED();
static void resetNetwork();

int main(int argc, char **argv) { 

    int i, rtrn;


    wiringPiSetup () ;

    /*
       for (i = 0 ; i < 6 ; ++i)
       pinMode (doPin[i], OUTPUT) ;
     */

    // DI 8 port
    for (i = 0 ; i < DI_PORT ; i++)
	pinMode (diPin[i], INPUT) ;

    // LED 2 port
    for (i = 0 ; i < LED_PORT ; i++)
	pinMode (ledPin[i], OUTPUT) ;

    // BUTTON 3 port
    for (i = 0 ; i < BUTTON_PORT ; i++)
	pinMode (buttonPin[i], INPUT) ;

    usleep(1000);
    int rt1, rt2, rt3;
    time_t  the_time;
    char buffer[255];
    char app[255];
    int resetCheck = 0;

    sprintf(app, "nohup %s &", APP_PATH);

    writeLogV2(LOGPATH, "[DeviceStatusCheck]", "Start\n");
    fflush(stdout);
    while(1)
    {
	time(&the_time);
	get_timef(the_time, buffer);

	rt1 = check_process(APP1_NAME);
	rt2 = check_process(APP2_NAME);
	rt3 = check_process(APP3_NAME);

	if(rt1 == 1 && rt2 == 1 && rt3 == 1)
	{
	    //printf("OK: %s\n", buffer);
	    digitalWrite(ledPin[0], 0); //blue on
	}
	else
	{
	    //printf("DIE: %s\n", buffer);

	    /** 새로 뛰움 **/
	    //system(app);
	    //printf("RELOAD: %s\n", buffer);
	    digitalWrite(ledPin[0], 1); //blue off
	}

	fflush(stdout);


	if( !digitalRead (buttonPin[0]) )
	{
	    //printf("reset Push\n");
	    resetCheck++;
	    if( resetCheck == 4 )
	    {
		writeLogV2(LOGPATH, "[DeviceStatusCheck]", "Factory Reset\n");
		resetCheck = 0;

		rtrn = system("cp -a /work/smart/config/* /work/config/");
		printf("cp -a /work/smart/config/* /work/config/ %d\n", rtrn);

		resetLED();

		resetNetwork();


		rtrn = system("/work/script/restartProgram.sh &");
		printf("/work/script/restartProgram.sh %d\n", rtrn);

	    }
	    //digitalWrite(ledPin[1], 0); //blue on
	}
	else
	{
	    //printf("Cancel\n");
	    resetCheck = 0;
	}

	/* 검사 후, process sleep */
	sleep(CHECK_SECOND);    
	    //digitalWrite(ledPin[0], !digitalRead (buttonPin[0])); //blue
    }

    writeLog( LOGPATH, "[DeviceStatusCheck] END" );
    return 0; 
} 

static void resetNetwork()
{
    DEVINFO	    device;
    int rtrn;

    ReadNetworkUseConfig(NETWORKUSEINFOPATH, &device.lanEnable, &device.wlanEnable );

    ReadOptionsConfig(OPTIONSINFOPATH, &device.lanAuto, &device.wlanAuto);
    if(device.lanEnable )
    {
	if( !device.lanAuto )
	{
	    rtrn = system("/work/script/network/lan/eth0.sh &");
	    printf("system() %d\n", rtrn);

	}
	else
	    rtrn = system("/work/script/network/lan/eth0Static.sh &");

    }

    if( device.wlanEnable )
    {
	if( !device.wlanAuto )
	{
	    rtrn = system("/work/script/network/wlan/wlan0.sh &");
	    printf("system() %d\n", rtrn);

	}
	else
	    rtrn = system("/work/script/network/wlan/wlan0Static.sh &");

    }


}
static void resetLED()
{
    int i = 0;
    digitalWrite(ledPin[0], 1); //blue off
    digitalWrite(ledPin[1], 1); //white off
    usleep(100000);

    for( i = 0; i < 5; i++ )
    {
	// on
	digitalWrite(ledPin[0], 0);
	digitalWrite(ledPin[1], 0);
	usleep(100000);

	// off
	digitalWrite(ledPin[0], 1);
	digitalWrite(ledPin[1], 1);
	usleep(100000);
    }
}

/** 프로세스 검사 */
int check_process(char* appName)
{   
    DIR* pdir;
    struct dirent *pinfo;
    int is_live = 0;

    pdir = opendir("/proc");
    if(pdir == NULL)
    {
	printf("err: NO_DIR\n");
	return 0;
    }

    /** /proc 디렉토리의 프로세스 검색 */
    while(1)
    {
	pinfo = readdir(pdir);
	if(pinfo == NULL)
	    break;

	/** 파일이거나 ".", "..", 프로세스 디렉토리는 숫자로 시작하기 때문에 아스키코드 57(9)가 넘을 경우 건너뜀 */
	if(pinfo->d_type != 4 || pinfo->d_name[0] == '.' || pinfo->d_name[0] > 57)
	    continue;

	FILE* fp;
	char buff[128];
	char path[128];

	sprintf(path, "/proc/%s/status", pinfo->d_name);
	fp = fopen(path, "rt");
	if(fp)
	{
	    fgets(buff, 128, fp);
	    fclose(fp);

	    /** 프로세스명과 status 파일 내용과 비교 */  
	    if(strstr(buff, appName)  )
	    {
		is_live = 1;
		break;
	    }
	}
	else
	{
	    printf("Can't read file [%s]\n", path);
	}
    }

    closedir(pdir);

    return is_live;
}

/** 현재 시간을 설정 */
void get_timef(time_t org_time, char *time_str)
{
    struct tm *tm_ptr;
    tm_ptr = localtime(&org_time);

    sprintf(time_str, "%d/%d/%d %d:%d:%d",
	    tm_ptr->tm_year+1900,
	    tm_ptr->tm_mon+1,
	    tm_ptr->tm_mday,
	    tm_ptr->tm_hour,
	    tm_ptr->tm_min,
	    tm_ptr->tm_sec);
}
