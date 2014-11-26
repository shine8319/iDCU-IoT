#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>

void getdoubledt(char *dt)
{
  struct timeval val;
  struct tm *ptm;

  gettimeofday(&val, NULL);
  ptm = localtime(&val.tv_sec);

  memset(dt , 0x00 , sizeof(dt));

  // format : YYMMDDhhmmssuuuuuu
  sprintf(dt, "%04d/%02d/%02d %02d:%02d:%02d.%03d"
      , ptm->tm_year + 1900, ptm->tm_mon + 1, ptm->tm_mday
      , ptm->tm_hour, ptm->tm_min, ptm->tm_sec
      , val.tv_usec/1000);
  printf("%s\n", dt);
}

int main()
{

	char strtime[128];

	while(1)
	{
	getdoubledt(strtime);
	usleep(1000*100);
	printf("test\n");
	}
	
	return 0;
}
