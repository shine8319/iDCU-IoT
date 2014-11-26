CC=/usr/local/arm/4.2.2-eabi/usr/bin/arm-linux-gcc
CPP=/usr/local/arm/4.2.2-eabi/usr/bin/arm-linux-g++
CCUSG=/opt/arm/3.4.1/bin/arm-linux-gcc
CC4600=/usr/local/hybus-arm-linux-R1.1/bin/arm-linux-gcc
RASCC=arm-linux-gnueabi-gcc
RASAR=arm-linux-gnueabi-ar
RASRANLIB=arm-linux-gnueabi-ranlib
RASCPP=arm-linux-gnueabi-cpp

SRC1 = serialsource.c
SRC2 = sfsock.c
SRC3 = sf.c
SRC4 = sfsource.c

LIB_EXPAT = -L../lib -lexpat
#LIB_EXPAT = -L/usr/local/arm/expat/lib -lexpat
LIB_SQLITE3 = -L../lib -lsqlite3
LIB_ETRIREG = -L../lib -lEtriReg
LIB_ETRIDEREG = -L../lib -lEtriDereg
LIB_PLCS_10 = -L./driver -lPLCS_10
LIB_PLCS_12 = -L./driver -lPLCS_12
LIB_PLCS_12ex = -L./driver -lPLCS_12ex
LIB_IDCU_IOT= -L./driver -liDCU_IoT
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
configRW = ./lib/configRW.c
writeLog = ./lib/writeLog.c
sqliteManager = ./lib/sqliteManager.c
TCPSocket = ./lib/TCPSocket.c
#ETRI_Registration = ./lib/ETRI_Registration.c
SQLite3Interface = ./lib/SQLite3Interface.c
libpointparser = ./lib/libpointparser.c
TARGET0 = ./bin
LIB_TARGET = ./lib
#PFLAGS += -DDEBUG
all:
#	$(CC) -c platform.c
#	$(CC) -c serialsource.c
#	$(CC) -c sf.c
#	$(CC) -c sfsock.c
#	$(CC) -c sfsource.c
	$(CC) -lpthread -o CommManager.o serialtonet.c $(SRC1) $(SRC2) $(SRC3) $(SRC4) $(LIB1) $(LIB_EXPAT)
	$(CC) -lpthread -o RealTimeDataManager.o RealTimeDataManager.c $(LIB1) $(LIB2) $(LIB_EXPAT)
#$(CC) -lpthread -o motelisten *.o

#uart_test:
#$(CC) -o uart_test.o uart_test.c uart_init.c 

#uart_read:
#$(CC) -o uart_read.o uart_read.c uart_init.c 
libEtriReg:
	$(RASCC) -c ./lib/ETRI_Registration.c ./lib/SQLite3Interface.c
	$(RASAR) rscv ../lib/libEtriReg.a ETRI_Registration.o SQLite3Interface.o

testhwclock:
	$(CC) -o testhwclock.o testhwclock.c
hello:
	$(RASCPP) -o hello_cc.o hello_cc.cpp

logwrite:
	$(CC) -lpthread -o logwrite.o logwrite.c
msgrcv:
	$(CC) -lpthread -o msgrcv.o msgrcv.c
msgsnd:
	$(CC) -lpthread -o msgsnd.o msgsnd.c
usnTowifi:
	$(CC) -lpthread -o usnTowifi.o usnTowifi.c

wifiTousn:
	$(CC) -lpthread -o wifiTousn.o wifiTousn.c

PointManager:
	$(CC) -lpthread -o PointManager.o PointManager.c $(LIB1) $(LIB3) $(LIB_EXPAT)

UartManager:
	$(RASCC) -o $(TARGET0)/UartManager.o UartManager.c $(writeLog) $(sqliteManager) $(LIB_SQLITE3) -lpthread

CommManager:
	$(RASCC) -lpthread -o $(TARGET0)/CommManager.o CommManager.c $(writeLog)

RealTimeDataManager:
	$(RASCC)  -o $(TARGET0)/RealTimeDataManager.o RealTimeDataManager.c $(TCPSocket) $(writeLog) $(LIB4) $(LIB5) $(LIB_SQLITE3) -lpthread

tagServer:
	$(RASCC) -c ./lib/ETRI_Registration.c ./lib/SQLite3Interface.c $(LIB_SQLITE3)
	$(RASAR) rc ../lib/libEtriReg.a ETRI_Registration.o SQLite3Interface.o
	$(RASRANLIB) ../lib/libEtriReg.a
	$(RASCC) -c ./lib/ETRI_Deregistration.c ./lib/SQLite3Interface.c $(LIB_SQLITE3)
	$(RASAR) rc ../lib/libEtriDereg.a ETRI_Deregistration.o SQLite3Interface.o
	$(RASRANLIB) ../lib/libEtriDereg.a
	$(RASCC)  -o $(TARGET0)/tagServer.o tagServer.c $(configRW) $(TCPSocket) $(writeLog) $(LIB_ETRIREG) $(LIB_ETRIDEREG) $(LIB_SQLITE3) -lpthread

M2MManager:
	$(RASCC) -lpthread -o $(TARGET0)/M2MManager.o M2MManager.c $(writeLog) $(LIB5) $(LIB6) $(LIB9) $(LIB11) $(LIB_SQLITE3)

IoTManager:
	$(RASCC) -o $(TARGET0)/IoTManager.o IoTManager.c $(writeLog) $(sqliteManager) $(LIB_SQLITE3) -lpthread

SetupManager:
	$(RASCC) -lpthread -o $(TARGET0)/SetupManager.o SetupManager.c $(writeLog) $(LIB5) $(LIB6) $(LIB9) $(configRW) $(LIB_SQLITE3) -I./include

PLCS-12:
	$(RASCC)  -o $(TARGET0)/PLCS-12.o PLCS-12.c $(TCPSocket) $(writeLog) $(configRW) $(libpointparser) $(LIB_SQLITE3) $(LIB_EXPAT) -I./include -lpthread

CommunicationManager:
	$(RASCC)  -o $(TARGET0)/CommunicationManager.o CommunicationManager.c $(TCPSocket) $(writeLog) $(configRW) $(libpointparser) $(LIB_PLCS_10) $(LIB_PLCS_12) $(LIB_PLCS_12ex) $(LIB_IDCU_IOT) $(LIB_EXPAT) -I./include -lpthread


iDCU-IoT:
	$(RASCC)  -o $(TARGET0)/iDCU-IoT.o iDCU-IoT.c $(TCPSocket) $(writeLog) $(configRW) $(LIB_SQLITE3) -I./include -lpthread
#$(RASCC) -lpthread -o $(TARGET0)/SetupManager.o SetupManager.c $(LIB5) $(LIB6) $(LIB9) $(LIB_SQLITE3) -I./include -L./lib -ludpconnectso

ETRI_simulator:
	$(RASCC)  -o $(TARGET0)/ETRI_simulator.o ETRI_simulator.c $(TCPSocket) $(writeLog) $(configRW) $(LIB_SQLITE3) -I./include -lpthread

test_server:
	gcc -D_REENTRANT test_server.c -o $(TARGET0)/test_server.o  -I./include -lpthread

test_client:
	gcc -D_REENTRANT test_client.c -o $(TARGET0)/test_client.o  -I./include -lpthread

UDPConnect:
	$(RASCC) -fPIC -c $(LIB_TARGET)/UDPConnect.c
	$(RASCC) -shared -o $(LIB_TARGET)/libudpconnectso.so.1.0.1 UDPConnect.o 

DBSchedule:
	$(RASCC) -lpthread -o $(TARGET0)/DBSchedule.o DBSchedule.c $(LIB4) $(LIB5) $(LIB_SQLITE3)

clean:
	rm *.o *.*~ -f
