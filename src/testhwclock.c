#include <stdio.h>
#include <unistd.h>

int main()
{
   //execl( "/bin/ls", "/bin/ls", "-al", "/tmp", NULL);
   //execl( "/sbin/hwclock", "/sbin/hwclock", "-w", NULL);
   execl( "/sbin/hwclock", "/sbin/hwclock", NULL);

   printf( "이 메시지가 보이면 지정된 프로그램이 \
없거나 어떤 문제로 실행되지 못한 것입니다.\n");
}
