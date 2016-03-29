RASCC=arm-linux-gnueabi-gcc
RASAR=arm-linux-gnueabi-ar
RASRANLIB=arm-linux-gnueabi-ranlib
RASCPP=arm-linux-gnueabi-g++

LIB_EXPAT = -L../../lib -lexpat
#LIB_EXPAT = -L/usr/local/arm/expat/lib -lexpat
LIB_SQLITE3 = -L../../lib -lsqlite3
#LIB_SQLITE3= -L/work/src/sqlite/sqlite-autoconf-3070800/compiled/lib -lsqlite3

configRW = ../lib/configRW.c
writeLog = ../lib/writeLog.c
sqliteManager = ../lib/sqliteManager.c
TCPSocket = ../lib/TCPSocket.c
#ETRI_Registration = ../lib/ETRI_Registration.c
SQLite3Interface = ../lib/SQLite3Interface.c
libpointparser = ../lib/libpointparser.c
stringTrim = ../lib/stringTrim.c
TARGET0 = ./bin
LIB_TARGET = ./lib
#PFLAGS += -DDEBUG
all:
	$(RASCC)  -c GN1200.c $(stringTrim) $(TCPSocket) $(writeLog) $(configRW) $(libpointparser) $(LIB_SQLITE3) $(LIB_EXPAT) -I./include -lpthread
	$(RASAR) rc libGN1200.a GN1200.o
	$(RASRANLIB) libGN1200.a

clean:
	rm *.o *.a *.*~ -f
