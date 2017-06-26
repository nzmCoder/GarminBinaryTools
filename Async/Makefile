CXXFLAGS =	-O1 -ggdb -Wall -fmessage-length=0
OBJS =		Async.o
LIBS =
CC = gcc

TARGET =	Async.exe

$(TARGET):	$(OBJS)
	$(CXX) -o $(TARGET) $(OBJS) $(LIBS)

all:	$(TARGET)

clean:
	rm -f $(OBJS) $(TARGET)
