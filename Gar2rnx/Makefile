CXXFLAGS =	-O1 -ggdb -Wall -fmessage-length=0
OBJS =		Gar2Rnx.o
LIBS =
CC = gcc

TARGET =	Gar2Rnx.exe

$(TARGET):	$(OBJS)
	$(CXX) -o $(TARGET) $(OBJS) $(LIBS)

all:	$(TARGET)

clean:
	rm -f $(OBJS) $(TARGET)
