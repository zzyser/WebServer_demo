CXX = g++
CFLAGS = -std=c++14 -O2 -Wall -g 

TARGET = server
OBJS = ../Log/*.cpp ../Pool/*.cpp ../Timer/*.cpp \
		../Http/*.cpp ../Server/*.cpp \
		../Buffer/*.cpp ../main.cpp

all: $(OBJS)
	$(CXX) $(CFLAGS) $(OBJS) -o ../bin/$(TARGET)  -pthread -lmysqlclient

clean:
	rm -rf ../bin/$(OBJS) $(TARGET)