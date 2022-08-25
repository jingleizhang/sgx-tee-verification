CC = gcc
C++ = g++
LINK = g++

LIBS = 
CCFLAGS = $(COMPILER_FLAGS) -c -g
C++FLAGS = $(COMPILER_FLAGS) -c -g -std=c++11

TARGET=sendgrid_test

INCLUDES = -I.

C++FILES = CJsonObject.cpp sendgridmsg.cpp main.cpp 
CFILES = cJSON.c

OBJFILE = $(CFILES:.c=.o) $(C++FILES:.cpp=.o)

all:$(TARGET)

$(TARGET): $(OBJFILE)
	$(LINK) $^ $(LIBS) -Wall -o $@

%.o:%.c
	$(CC) -o $@ $(CCFLAGS) $< $(INCLUDES)

%.o:%.cpp
	$(C++) -o $@ $(C++FLAGS) $< $(INCLUDES)

install:
	tsxs -i -o $(TARGET)

clean:
	rm -rf $(TARGET)
	rm -rf $(OBJFILE)