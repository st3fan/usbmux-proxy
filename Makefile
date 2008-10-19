
CC=g++
CFLAGS=-c -Wall -I/usr/local/include/boost-1_36
LDFLAGS=-L/usr/local/include -lboost_system-xgcc40-mt-1_36
SOURCES = usbmux.cpp usbmux-proxy.cpp
OBJECTS=$(SOURCES:.cpp=.o)
EXECUTABLE =usbmux-proxy

all: $(SOURCES) $(EXECUTABLE)

$(EXECUTABLE): $(OBJECTS)
	$(CC) $(LDFLAGS) $(OBJECTS) -o $@

.cpp.o:
	$(CC) $(CFLAGS) $< -o $@
