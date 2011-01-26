# AsteroidBlaster Makefile by Sterling Hirsh
# Uses uname to decide whether it's on Linux or OSX so it can tell which libs to include.

UNAME=$(shell uname)
ifeq ($(UNAME), Linux)
   PLATFORMSPECIFICCFLAGS=-I/usr/include/SDL -D_GNU_SOURCE=1 -D_REENTRANT
#-I/home/rkudo/library/SDL_ttf-2.0.10/
   PLATFORMSPECIFICLDFLAGS=-lGL -lGLU -lglut -lSDL -lpthread 
#-lSDL_ttf -L/home/rkudo/library/SDL_ttf-2.0.10/.libs/
else
   PLATFORMSPECIFICCFLAGS=-I/usr/local/include/SDL -D_GNU_SOURCE=1 -D_THREAD_SAFE
   PLATFORMSPECIFICLDFLAGS=-framework GLUT -framework OpenGL -L/usr/local/lib -lSDLmain -lSDL -Wl,-framework,Cocoa

endif

LDFLAGS=$(PLATFORMSPECIFICLDFLAGS) -g

# -I. -iquote makes it so quoted #includes look in ./
# -Wall makes warnings appear
# -c makes .o files
CFLAGS=$(PLATFORMSPECIFICCFLAGS) -O3 -I. -iquote -Wall -c -g
CC=g++

PROGNAME=AsteroidBlaster

UTILITYFILES=Utility/Vector3D.cpp Utility/Object3D.cpp Utility/BitmapTextDisplay.cpp Utility/GameState.cpp
GRAPHICSFILES=Graphics/GlutUtility.cpp Graphics/Mesh3D.cpp Graphics/MeshPoint.cpp Graphics/Skybox.cpp Graphics/Sprite.cpp Graphics/TextureImporter.cpp Graphics/MeshImporter.cpp Graphics/Camera.cpp
ITEMSFILES=Items/Asteroid3D.cpp Items/AsteroidShip.cpp Items/BoundingSpace.cpp Items/Player.cpp Items/Enemy.cpp
SHOTSFILES=Shots/AsteroidShot.cpp Shots/AsteroidShotBeam.cpp
AIFILES=
NETWORKFILES=
FILES=main.cpp $(UTILITYFILES) $(GRAPHICSFILES) $(ITEMSFILES) $(SHOTSFILES) $(AIFILES) $(NETWORKFILES)

OBJECTS=$(FILES:.cpp=.o)

#all:
#	g++ ${CFLAGS} -o ${PROGNAME} ${FILES}

all: $(FILES) $(PROGNAME)

$(PROGNAME): $(OBJECTS)
	$(CC) $(LDFLAGS) $(OBJECTS) -o $@

.cpp.o:
	$(CC) $(CFLAGS) $< -o $@


run:
	./${PROGNAME}

turnin:
	handin zwood csc471finals2 *.cpp *.h *.bmp README
	handin zwood csc471finalwebs2 asteroidssite/*
	handin zwood csc471finals2
	handin zwood csc471finalwebs2

valgrind:
	valgrind --leak-check=full ./${PROGNAME}

gdb:
	gdb ./${PROGNAME}

clean:
	rm -f *.o */*.o ${PROGNAME} ${PROGNAME}.dSYM
