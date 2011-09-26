# AsteroidBlaster Makefile by Sterling Hirsh
# Uses uname to decide whether it's on Linux or OSX so it can tell which libs to include.

UNAME=$(shell uname)
ifeq ($(UNAME), Linux)
   # Linux stuff
   # Use this if you're on a school machine.
	# We have to use this version on linux since the -mt version doesn't exist.
   SDL_LIBS=$(shell "sdl-config" "--libs")
   SDL_CFLAGS=$(shell "sdl-config" "--cflags")
   PLATFORMSPECIFICCFLAGS=-I./Libraries/SDL_ttf-2.0.10 -I./Libraries/SDL_image-1.2.10 

   PLATFORMSPECIFICLDFLAGS= -L./Libraries/SDL_ttf-2.0.10/.libs -L./Libraries/SDL_image-1.2.10/.libs -Wl,-rpath=./Libraries/glew-1.5.8/lib -Wl,-rpath=./Libraries/SDL_ttf-2.0.10/.libs -Wl,-rpath=./Libraries/SDL_image-1.2.10/.libs  -lGL -lGLU -lSDL -lGLEW -lpthread
else
   # Mac stuff
   SDL_LIBS=$(shell "/sw/bin/sdl-config" "--libs")
   SDL_CFLAGS=$(shell "/sw/bin/sdl-config" "--cflags")
   PLATFORMSPECIFICCFLAGS=-I /opt/local/include
   PLATFORMSPECIFICLDFLAGS=-framework OpenGL -Wl,-framework,Cocoa -Wl
endif

LDFLAGS=$(PLATFORMSPECIFICLDFLAGS) $(SDL_LIBS) -lSDL_image -lSDL_ttf -g -O0 -LLibraries/fmod -lfmodex
# -I. -iquote makes it so quoted #includes look in ./
# -Wall makes warnings appear
# -c makes .o files

FMODLIB_PATH          = Libraries/fmod
FMODLIB_NAME_RELEASE  = libfmodex.dylib
FMODLIB_NAME_LOGGING  = libfmodexL.dylib

CFLAGS=$(PLATFORMSPECIFICCFLAGS) -I. -c $(SDL_CFLAGS) -g -O0
CC=g++

PROGNAME=AsteroidBlaster

UTILITYFILES=Utility/Vector3D.cpp Utility/GameState.cpp Utility/Custodian.cpp Utility/InputManager.cpp Utility/Point3D.cpp Utility/Radar.cpp Utility/Quaternion.cpp Utility/ViewFrustum.cpp Utility/Matrix4.cpp Utility/GlobalUtility.cpp Utility/SoundEffect.cpp Utility/GameSettings.cpp Utility/CollisionTypes.cpp Utility/Timer.cpp

MENUFILES=Menus/Menu.cpp Menus/MainMenu.cpp Menus/StoreMenu.cpp Menus/CreditsMenu.cpp Menus/SettingsMenu.cpp Menus/HelpMenu.cpp

GRAPHICSFILES=Graphics/Mesh3D.cpp Graphics/MeshPoint.cpp Graphics/Skybox.cpp Graphics/Texture.cpp Graphics/Sprite.cpp Graphics/Camera.cpp Graphics/MeshFace.cpp Graphics/Image.cpp

ITEMSFILES=Items/Drawable.cpp Items/Object3D.cpp Items/Asteroid3D.cpp Items/AsteroidShip.cpp Items/BoundingSpace.cpp Items/Ring.cpp Items/Shard.cpp Items/GlowSquare.cpp Items/BoundingWall.cpp Items/Ball.cpp Items/Spring.cpp

SHOTSFILES=Shots/Shot.cpp Shots/BeamShot.cpp Shots/BlasterShot.cpp Shots/TractorBeamShot.cpp Shots/ElectricityShot.cpp Shots/ExplosiveShot.cpp Shots/EnergyShot.cpp Shots/TimedBombShot.cpp Shots/RemoteBombShot.cpp Shots/HomingMissileShot.cpp Shots/RamShot.cpp
# UNUSED: Shots/RamShot.cpp

AIFILES=AI/FlyingAI.cpp AI/ShootingAI.cpp 

NETWORKFILES=Network/ClientCommand.cpp
# Network/UDP_Server.cpp Network/UDP_Client.cpp Network/ClientNode.cpp Network/NetTimer.cpp Network/NetShard.cpp Network/NetAsteroid.cpp Network/NetShip.cpp Network/NetBlasterShot.cpp Network/NetBeamShot.cpp Network/NetTractorBeamShot.cpp

WEAPONSFILES=Weapons/Blaster.cpp Weapons/RailGun.cpp Weapons/Weapon.cpp Weapons/TractorBeam.cpp Weapons/Electricity.cpp Weapons/TimedBomber.cpp Weapons/Energy.cpp Weapons/RemoteBomber.cpp Weapons/HomingMissile.cpp Weapons/Ram.cpp


HUDFILES=HUD/ProgressBar.cpp HUD/Minimap.cpp HUD/Screen.cpp HUD/WeaponDisplay.cpp

PARTICLEFILES=Particles/Particle.cpp Particles/BlasterShotParticle.cpp Particles/EngineParticle.cpp Particles/BlasterImpactParticle.cpp Particles/TractorAttractionParticle.cpp Particles/ElectricityImpactParticle.cpp Particles/ShardParticle.cpp

TEXTFILES=Text/Text.cpp Text/GameMessage.cpp Text/Input.cpp

FILES=$(UTILITYFILES) $(MENUFILES) $(GRAPHICSFILES) $(ITEMSFILES) $(SHOTSFILES) $(AIFILES) $(NETWORKFILES) $(WEAPONSFILES) $(HUDFILES) $(PARTICLEFILES) $(TEXTFILES)

OBJECTS=$(FILES:.cpp=.o)

all: $(FILES) AsteroidBlaster

AsteroidBlaster: AsteroidBlaster.o $(OBJECTS)
	$(CC) $(LDFLAGS) AsteroidBlaster.o $(OBJECTS) -o $@
	install_name_tool -change ./${FMODLIB_NAME_RELEASE} ${FMODLIB_PATH}/${FMODLIB_NAME_RELEASE} AsteroidBlaster

AsteroidBlaster.o: AsteroidBlaster.cpp
	$(CC) $(CFLAGS) $< -o $@

.cpp.o:
	$(CC) $(CFLAGS) $< -o $@


run:
	./${PROGNAME}

runServer:
	./${PROGNAME} -s 5001

runClient:
	./${PROGNAME} -c localhost 5001

lib:
	./lib.sh make

libClean:
	./lib.sh clean

valgrind:
	valgrind --leak-check=no --track-origins=yes --log-file=valgrindOutput ./${PROGNAME}

gdb:
	gdb ./${PROGNAME}

clean:
	rm -f *.o */*.o ${PROGNAME} ${PROGNAME}.dSYM Client Server

gprof:
	gprof ${PROGNAME} gmon.out | less

killall:
	killall -9 ${PROGNAME}

glc-capture:
	glc-capture -s -o ${PROGNAME}.glc ./${PROGNAME}

glc-play:
	glc-play ${PROGNAME}.glc

glc-encode:
	glc-play ${PROGNAME}.glc -y 1 -o - | mencoder -demuxer y4m - -ovc lavc -lavcopts vcodec=mpeg4:vbitrate=1500 -o video.avi

