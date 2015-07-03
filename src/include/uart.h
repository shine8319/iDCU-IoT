//int OpenSerial(char *baud, char databits);
int OpenSerial(char *baud, char parity, char databits, char stopbits );
int tty_raw(int fd, char *baud, char flowc, char databits, char parity, char stopbits);
int CloseSerial(int fd);

int getBaudrate(char *baud);
int getDatabit(char databit );

UINT8 getStopbit(UINT8 stopbit);
UINT8 getParity(UINT8 parity);
