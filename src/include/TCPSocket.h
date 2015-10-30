
INT32 TCPServer(INT32 port);
INT32 TCPClient(INT8 *serverIP, UINT16 port );

int sender( int *tcp, UINT8 *packet, UINT32 size );
int receive( int *tcp, int timeout );
