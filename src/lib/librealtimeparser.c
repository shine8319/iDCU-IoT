#include <stdio.h> 
#include <string.h>


#include "../include/RealTimeDataManager.h"

Return_RTDParser dispens;


Return_RTDParser RTDParser( char nodeid, char *addr, char size )
{

	memcpy( dispens.parsingdata, addr, size );
	printf("NodeID = %d, Data length = %d\n", nodeid, strlen(dispens.parsingdata) );
	if( strlen(dispens.parsingdata) == 0 )
	{
		dispens.nodeid = nodeid;
		dispens.rtrn = -1;
		return dispens;
	}
	else 
	{
		dispens.temp= strtok( dispens.parsingdata, ";" );
		//printf("temp:%s ", dispens.temp);
		dispens.humi = strtok( NULL, ";");
		//printf("humi:%s ", dispens.humi);
		dispens.lux = strtok( NULL, ";");
		//printf("lux:%s ", dispens.lux);
		dispens.bat = strtok( NULL, ";");
		//printf("bat:%s\n", dispens.bat);
		dispens.seq = strtok( NULL, ";");

		dispens.nodeid = nodeid;
		dispens.rtrn = 0;
	}

	return dispens;

}
