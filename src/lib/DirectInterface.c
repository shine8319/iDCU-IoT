#include <stdio.h>
#include <string.h>
#include <stdlib.h>

#include "../include/iDCU.h"
#include "../include/parser/driverInfoParser.h"
#include "../include/parser/directInterfaceInfoParser.h"
#define DRIVERINFOPATH "/work/config/driver_info.xml"	// add 2016.03.22 for machine direct interface test

int getDriverList(unsigned char* sendBuffer)
{

    int i = 0;
    int offset = 6;
    DRIVERLIST list;
    memset( &list, 0, sizeof(DRIVERLIST) );

    list = driverInfoParser(DRIVERINFOPATH);

    printf("driver count %d\n", list.getListCount);

    for( i = 0; i < list.getListCount; i++ )
    {
	sendBuffer[offset++] = atoi( list.driver[i].id );
    }

    return list.getListCount;
}


DIRECTINTERFACEINFO getDirectInterfaceInfo(const char *path)
{
    DIRECTINTERFACEINFO item;
    
    item = directInterfaceInfoParser(path);
    
    return item;
}
