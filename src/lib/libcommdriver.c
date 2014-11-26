#include <stdio.h>
#include "../include/sfsock.h"
#include "../include/CommManager.h"

/*
typedef struct COMMPARSING {
	uint16_t bat;
	uint16_t temp;
	uint16_t humi;
	uint16_t lux;
} COMMPARSING;
*/

	

COMMPARSING CommDriver( unsigned char *pack, int size )
{
	COMMPARSING rtrn;

	//uint16_t nodeid = (255*pack[NODEID]+pack[NODEID+1]);
	uint16_t nodeid = pack[NODEID];
	uint16_t reading =  (255*pack[BAT]+pack[BAT+1]);
	uint16_t temperature = (255*pack[TEMP]+pack[TEMP+1]);
	uint16_t humidity = (255*pack[HUMI]+pack[HUMI+1]);
	uint16_t tsr = (255*pack[LUX]+pack[LUX+1]); 

	reading	= reading - 2200;
	printf(" reading - 2200 = %d\n", reading);
	reading /= 8;
	printf("Temp. = %.1f, Humi. = %.1f, Lux. = %d, Bat. = %d\n",
			-39.6 + 0.01 * temperature,
			((-39.6 + 0.01 * temperature) - 25) * ( 0.01 + 0.00008 * humidity ) + (-4 + 0.0405 * humidity + (-0.0000028) * humidity * humidity ),
			tsr,
			reading
			);

	rtrn.nodeid = nodeid;
	rtrn.bat = reading;
	rtrn.temp = (-39.6 + 0.01 * temperature) * 10.0;
	rtrn.humi = ( ((-39.6 + 0.01 * temperature) - 25) * ( 0.01 + 0.00008 * humidity ) + (-4 + 0.0405 * humidity + (-0.0000028) * humidity * humidity ) ) * 10.0;
	rtrn.lux = tsr;
	/*
	for(i = 0; i < tag.getPointSize; i++)
	{
		if( tag.id[i] == (255*pack[14]+pack[13]) )
		{
			printf("Mach NodeID = %d ( group = %d )\n", tag.id[i], pack[10]);
			sprintf( (char *)point[i].addr, "%d;%d;%d;%d;%d;", pack[10],(int)temperature/10,(int)humidity/10,(int)tsr,(int)reading/10);
		}
	}
	*/

	return rtrn;


}
