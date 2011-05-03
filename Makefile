#CC = g++

LDLIBS = -lopenal -logg -lvorbis -lvorbisfile -lglut -lGLU -lGL
LDFLAGS = -g -L/home/csunix/trev/shoot/lib
WARNFLAGS = -Wall -Wno-deprecated
CXXFLAGS = -g -I/home/csunix/trev/shoot/include $(WARNFLAGS)

all: main

main: main.o ogg.o soundmanager.o menu.o

main.o: main.cpp ogg.h menu.h

ogg.o: ogg.h ogg.cpp

soundmanager.o: soundmanager.cpp soundmanager.h

menu.o: menu.cpp menu.h

clean:
	rm -f main main.o ogg.o soundmanager.o

