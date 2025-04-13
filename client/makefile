CC=g++
CCLAGS=-c -Wall -DNDEBUG -std=c++17
LDFLAGS= -static -lcrypto++  
SOURCES=$(wildcard *.cpp)
HEADERS=$(wildcard *.h)
OBJECTS=$(SOURCES:.cpp=.o)
EXECUTABLE=client

all: $(SOURCES) $(HEADERS) $(EXECUTABLE)
	
$(EXECUTABLE): $(OBJECTS) 
	$(CC) $(OBJECTS) $(LDFLAGS) -o $@

.cpp.o:
	$(CC) $(CCLAGS) $< -o $@
	
clean:
	rm -rf *.o $(Target)
	rm -rf *.app $(Target)
	rm client

