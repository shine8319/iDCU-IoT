#include <stdio.h> 
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

#include "../include/iDCU.h"
#include "../include/hiredis/hiredis.h"
#include "../include/hiredis/async.h"
#include "../include/hiredis/adaters/libevent.h"


#define BUFFER_SIZE 1024

redisContext* redisInitialize() {
    
    redisContext* c;
    const char *hostname = "127.0.0.1";
    int port = 6379;
    struct timeval timeout = { 1, 500000 }; // 1.5 seconds

    c = redisConnectWithTimeout(hostname, port, timeout);

    /*
    if (c == NULL || c->err) {

	writeLogV2("/work/smart/log/", "[SetupManager]", "DB Fail" );

        if (c) {
	    debugPrintf(configInfo.debug, "Connection error: %s\n", c->errstr);
            redisFree(c);
        } else {
	    debugPrintf(configInfo.debug, "Connection error: can't allocate redis context\n");
        }
        exit(1);
    }
    else
	writeLogV2("/work/smart/log/", "[SetupManager]", "DB OK" );
	*/

    return c;

}


