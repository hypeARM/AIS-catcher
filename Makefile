SRC = Main.cpp IO.cpp DSP.cpp AIS.cpp Model.cpp Utilities.cpp Demod.cpp DeviceRTLSDR.cpp DeviceAIRSPYHF.cpp DeviceAIRSPY.cpp DeviceFileRAW.cpp DeviceFileWAV.cpp DeviceSDRPLAY.cpp DeviceRTLTCP.cpp
OBJ = Main.o IO.o DSP.o AIS.o Model.o Utilities.o Demod.o DeviceRTLSDR.o DeviceAIRSPYHF.o DeviceAIRSPY.o DeviceFileRAW.o DeviceFileWAV.o DeviceSDRPLAY.o DeviceRTLTCP.o

CC = gcc
override CFLAGS += -Ofast -Wno-psabi -std=c++11
override LFLAGS += -lstdc++ -lm -o AIS-catcher 

CFLAGS_RTL = -DHASRTLSDR 
CFLAGS_AIRSPYHF = -DHASAIRSPYHF 
CFLAGS_AIRSPY = -DHASAIRSPY 
CFLAGS_SDRPLAY = -DHASSDRPLAY 
CFLAGS_RTLTCP = -DHASRTLTCP 

LFLAGS_RTL = -lrtlsdr -lpthread
LFLAGS_AIRSPYHF = -lairspyhf -lpthread
LFLAGS_AIRSPY = -lairspy -lpthread
LFLAGS_SDRPLAY = -lsdrplay_api -lpthread
LFLAGS_RTLTCP = -lpthread

all: lib
	$(CC) $(OBJ) $(LFLAGS_AIRSPYHF) $(LFLAGS_AIRSPY) $(LFLAGS_RTL) $(LFLAGS) $(LFLAGS_RTLTCP)

rtl-only: lib-rtl
	$(CC) $(OBJ) $(LFLAGS) $(LFLAGS_RTL)

airspyhf-only: lib-airspyhf
	$(CC) $(OBJ) $(LFLAGS) $(LFLAGS_AIRSPYHF)

airspy-only: lib-airspy
	$(CC) $(OBJ) $(LFLAGS) $(LFLAGS_AIRSPY)

sdrplay-only: lib-sdrplay
	$(CC) $(OBJ) $(LFLAGS) $(LFLAGS_SDRPLAY)

rtltcp-only: lib-rtltcp
	$(CC) $(OBJ) $(LFLAGS) $(LFLAGS_RTLTCP)

lib: 
	$(CC) -c $(SRC) $(CFLAGS) $(CFLAGS_AIRSPYHF) $(CFLAGS_AIRSPY) $(CFLAGS_RTL) $(CFLAGS_RTLTCP)

lib-rtl:
	$(CC) -c $(SRC) $(CFLAGS) $(CFLAGS_RTL)

lib-airspyhf:
	$(CC) -c $(SRC) $(CFLAGS) $(CFLAGS_AIRSPYHF)

lib-airspy:
	$(CC) -c $(SRC) $(CFLAGS) $(CFLAGS_AIRSPY)

lib-sdrplay:
	$(CC) -c $(SRC) $(CFLAGS) $(CFLAGS_SDRPLAY)

lib-rtltcp:
	$(CC) -c $(SRC) $(CFLAGS) $(CFLAGS_RTLTCP)

clean:
	rm *.o 
	rm AIS-catcher

install:
	cp AIS-catcher /usr/local/bin/AIS-catcher
