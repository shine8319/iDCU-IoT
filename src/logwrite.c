#include <stdio.h>
#include <string.h>

int main( void)
{
	FILE    *fp_dest;    // ���� ����� ���� ���� ��Ʈ�� ������
	char     buff[1024]; // ���� ������ �б�/���⸦ ���� ����
	size_t   n_size;     // �аų� ���⸦ ���� �������� ����
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
