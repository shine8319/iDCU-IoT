#include <unistd.h>
#include <stdio.h>
#include <string.h>
#include <sys/socket.h>
#include <sys/ioctl.h>
#include <sys/stat.h>
#include <netinet/in.h>
#include <net/if.h>
#include <arpa/inet.h>

int s_getIpAddress (const char * ifr, unsigned char * out) {
    int sockfd;
    struct ifreq ifrq;
    struct sockaddr_in * sin;
    sockfd = socket(AF_INET, SOCK_DGRAM, 0);
    strcpy(ifrq.ifr_name, ifr);
    if (ioctl(sockfd, SIOCGIFADDR, &ifrq) < 0) {
	perror( "ioctl() SIOCGIFADDR error");
	return -1;
    }
    sin = (struct sockaddr_in *)&ifrq.ifr_addr;
    memcpy (out, (void*)&sin->sin_addr, sizeof(sin->sin_addr));

    close(sockfd);

    return 4;
}

int s_getMacAddress (const char * ifr, unsigned char * out) {
    int sockfd;
    struct ifreq ifrq;
    sockfd = socket(AF_INET, SOCK_DGRAM, 0);
    strcpy(ifrq.ifr_name, ifr);
    if (ioctl(sockfd, SIOCGIFHWADDR, &ifrq) < 0) {
	perror( "ioctl() SIOCGIFADDR error");
	return -1;
    }
    memcpy (out, (unsigned char*)&ifrq.ifr_hwaddr.sa_data, sizeof(ifrq.ifr_hwaddr.sa_data));

    close(sockfd);

    return 4;
}

