RASCC=arm-linux-gnueabi-gcc
RASAR=arm-linux-gnueabi-ar
RASRANLIB=arm-linux-gnueabi-ranlib
RASCPP=arm-linux-gnueabi-g++

LIB_EXPAT = -L../../lib -lexpat
#LIB_EXPAT = -L/usr/local/arm/expat/lib -lexpat
LIB_SQLITE3 = -L../../lib -lsqlite3
LIB_ETRIREG = -L../../lib -lEtriReg
LIB_ETRIDEREG = -L../../lib -lEtriDereg
#LIB_SQLITE3= -L/work/src/sqlite/sqlite-autoconf-3070800/compiled/lib -lsqlite3
LIB1 = ./lib/libpointparser.c
LIB2 = ./lib/librealtimeparser.c
LIB3 = ./lib/libreleasesharedmemory.c
LIB4 = ./lib/librtdprocess.c
LIB5 = ./lib/libsqlite3manage.c
LIB6 = ./lib/libm2mprocess.c

LIB7 = ./lib/libcommdriver.c
LIB8 = ./lib/libdeviceparser.c
LIB9 = ./lib/libm2mprotocol.c
LIB10 = ./lib/libcommprocess.c

LIB11 = ./lib/getDiffTime.c
configRW = ../lib/configRW.c
writeLog = ../lib/writeLog.c
sqliteManager = ../lib/sqliteManager.c
TCPSocket = ../lib/TCPSocket.c
#ETRI_Registration = ../lib/ETRI_Registration.c
SQLite3Interface = ../lib/SQLite3Interface.c
libpointparser = ../lib/libpointparser.c
TARGET0 = ./bin
LIB_TARGET = ./lib
#PFLAGS += -DDEBUG
all:
	$(RASCC)  -c PLCS_12ex.c $(TCPSocket) $(writeLog) $(configRW) $(libpointparser) $(LIB_SQLITE3) $(LIB_EXPAT) -I./include -lpthread
	$(RASAR) rc libPLCS_12ex.a PLCS_12ex.o
	$(RASRANLIB) libPLCS_12ex.a
	$(RASCC)  -c PLCS_12.c $(TCPSocket) $(writeLog) $(configRW) $(libpointparser) $(LIB_SQLITE3) $(LIB_EXPAT) -I./include -lpthread
	$(RASAR) rc libPLCS_12.a PLCS_12.o
	$(RASRANLIB) libPLCS_12.a
	$(RASCC)  -c PLCS_10.c $(TCPSocket) $(writeLog) $(configRW) $(libpointparser) $(LIB_SQLITE3) $(LIB_EXPAT) -I./include -lpthread
	$(RASAR) rc libPLCS_10.a PLCS_10.o
	$(RASRANLIB) libPLCS_10.a
	$(RASCC)  -c iDCU_IoT.c $(TCPSocket) $(writeLog) $(configRW) $(libpointparser) $(LIB_SQLITE3) $(LIB_EXPAT) -I./include -lpthread
	$(RASAR) rc libiDCU_IoT.a iDCU_IoT.o
	$(RASRANLIB) libiDCU_IoT.a

clean:
	rm *.o *.*~ -f
