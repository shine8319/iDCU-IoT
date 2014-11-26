#include <stdio.h>
#include <string.h>

int main( void)
{
	FILE    *fp_dest;    // 복사 대상을 위한 파일 스트림 포인터
	char     buff[1024]; // 파일 내요을 읽기/쓰기를 위한 버퍼
	size_t   n_size;     // 읽거나 쓰기를 위한 데이터의 개수
	int i = 0, position;

	//fseek(fp_dest, -1, SEEK_END);


	//for( i = 0; i < 5; i++ )
	while(1)
	{
		fp_dest  = fopen( "./comm.log", "a");
		sprintf( buff, "ndx=%d, Austem M2MDevice log test\n", i++);
		//sprintf( buff, "ndx=%d, Austem M2MDevice log test\n", i);
		fwrite( buff, 1, strlen(buff), fp_dest);
		fclose( fp_dest);
		sleep(1);
	}                            

	return 0;
}
