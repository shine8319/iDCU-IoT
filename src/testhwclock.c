#include <stdio.h>
#include <unistd.h>

int main()
{
   //execl( "/bin/ls", "/bin/ls", "-al", "/tmp", NULL);
   //execl( "/sbin/hwclock", "/sbin/hwclock", "-w", NULL);
   execl( "/sbin/hwclock", "/sbin/hwclock", NULL);

   printf( "�� �޽����� ���̸� ������ ���α׷��� \
���ų� � ������ ������� ���� ���Դϴ�.\n");
}
