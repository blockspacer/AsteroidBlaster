/**
 * Asteroid Blaster -- IN 3D!
 * Graphics Team: Sterling Hirsh, Taylor Arnicar, Ryuho Kudo, Jake Juszak, Chris Brenton
 * AI Team: Taylor Arnicar, Mike Smith, Sean Ghiocel, Justin Kuehn
 * Final Project for CPE 476 - Winter & Spring 2011
 */
#include <math.h>
#include <list>
#include <sstream>

#include "Utility/GlobalUtility.h"
#include "Utility/Point3D.h"
#include "Items/Asteroid3D.h"
#include "Items/AsteroidShip.h"
#include "Graphics/Skybox.h"
#include "Graphics/Sprite.h"
#include "Graphics/Camera.h"
#include "Particles/Particle.h"
#include "Items/BoundingSpace.h"
#include "Text/Text.h"
#include "Text/GameMessage.h"
#include "Utility/GameState.h"
#include "Utility/InputManager.h"
#include "Utility/Matrix4.h"
#include "Utility/SoundEffect.h"
#include "Graphics/Texture.h"
#include "Graphics/Image.h"

#include "Menus/MainMenu.h"
#include "Menus/StoreMenu.h"
#include "Menus/SettingsMenu.h"
#include "Menus/HelpMenu.h"
#include "Menus/CreditsMenu.h"
#include "Menus/HighScoreList.h"
#include "Text/Input.h"


#include "Network/NetUtility.h"
#include "Network/ClientSide.h"
#include "Network/ServerSide.h"

#include "SDL.h"

#include <libgen.h>

// DEBUG
#include <fstream>
std::ofstream debugoutput("debug.txt");


GameState* gameState;
// the absolute time update was last called
static double lastUpdateTime = 0;

void init() {
   // Initialize the SDL video/audio system
   if(SDL_Init(SDL_INIT_AUDIO|SDL_INIT_VIDEO)<0) {
      std::cerr << "Failed to initialize SDL Video/Audio!" << std::endl;
      exit(1);
   }

   // Tell system which functions to process when exit() call is made
   // This cleans up and fixes the monitor resolution if in full screen.
   atexit(SDL_Quit);

   // Get optimal video settings
   vidinfo = SDL_GetVideoInfo();

   if (!vidinfo) {
      std::cerr << "Couldn't get video settings information! " << SDL_GetError() << std::endl;
      exit(1);
   }

   if (TTF_Init()) {
      std::cerr << TTF_GetError() << std::endl;
      exit(1);
   }

   SoundEffect::Init();

   // Set opengl attributes
   SDL_GL_SetAttribute(SDL_GL_RED_SIZE,      5);
   SDL_GL_SetAttribute(SDL_GL_GREEN_SIZE,    5);
   SDL_GL_SetAttribute(SDL_GL_BLUE_SIZE,     5);
#ifdef __APPLE__
   SDL_GL_SetAttribute(SDL_GL_DEPTH_SIZE,    32);
#else
   SDL_GL_SetAttribute(SDL_GL_DEPTH_SIZE,    16);
#endif
   SDL_GL_SetAttribute(SDL_GL_DOUBLEBUFFER,  1);

   setupVideo();

   // Set the timer
   SDL_Init(SDL_INIT_TIMER);

   // Disable the cursor
   SDL_ShowCursor(SDL_DISABLE);

   // Grab the input
   // Disabled for debug purposes.
   SDL_WM_GrabInput(SDL_GRAB_OFF);

   // Set the title
   SDL_WM_SetCaption("Asteroid Blaster", 0);


   //setup glew and GLSL
#ifndef __APPLE__
#ifndef ATSCHOOL
   glewInit();
   if (GLEW_ARB_vertex_shader && GLEW_ARB_fragment_shader &&
         GL_EXT_geometry_shader4)
      printf("Ready for GLSL\n");
   else {
      printf("Not enough GLSL support\n");
      exit(1);
   }
   printf("GL multitexturing: %d\n", GL_ARB_multitexture);
   printf("GLEW multitexturing: %d\n", GLEW_ARB_multitexture);

#endif
#endif


   glPixelStorei(GL_UNPACK_ALIGNMENT, 4);

   //GL setting calls
   //This enables alpha transparency so that things are see through
   glEnable(GL_BLEND);

   hudFont = TTF_OpenFont("Fonts/Slider.ttf", 18);
   if(!hudFont)
   {
      printf("TTF_OpenFont: %s\n", TTF_GetError());
      exit(3);
   }

   menuFont = TTF_OpenFont("Fonts/Slider.ttf", 24);
   if(!menuFont)
   {
      printf("TTF_OpenFont: %s\n", TTF_GetError());
      exit(3);
   }

   //set the background to be black
   glClearColor(0.0, 0.0, 0.0, 1.0);
   //glClearColor(1.0, 1.0, 1.0, 1.0);

   //initialize some GL stuff

   // enables lighting so that minimap's sphere can have color
   glEnable(GL_LIGHTING);

   glEnable(GL_LINE_SMOOTH);
   //glEnable(GL_MULTISAMPLE_ARB);
   //glHint(GL_LINE_SMOOTH_HINT, GL_PERSPECTIVE_CORRECTION_HINT);
   glHint(GL_LINE_SMOOTH_HINT, GL_NICEST);

   //initialize light
   //GLfloat headlight_pos[4] = {(float)(WORLD_SIZE / 2.0f), (float)(WORLD_SIZE / 2.0f), (float)(WORLD_SIZE / 2.0f), 1.0f};
   //GLfloat headlight_amb[4] = {0.1f, 0.1f, 0.1f, 1.0f};
   //GLfloat headlight_diff[4] = {1, 1, 1, 1.0};
   //GLfloat headlight_spec[4] = {1, 1, 1, 1.0};

   GLfloat minimaplight_pos[4] = {0, 0, 0, 1};
   GLfloat minimaplight_amb[4] = {1, 1, 1, 0.5};
   GLfloat minimaplight_diff[4] = {1, 1, 1, 0.5};
   GLfloat minimaplight_spec[4] = {1, 1, 1, 0.5};


   glEnable(GL_LIGHT1);
   glLightfv(GL_LIGHT1, GL_AMBIENT, minimaplight_amb);
   glLightfv(GL_LIGHT1, GL_DIFFUSE, minimaplight_diff);
   glLightfv(GL_LIGHT1, GL_SPECULAR, minimaplight_spec);
   glLightfv(GL_LIGHT1, GL_POSITION, minimaplight_pos);

   //initialize textures
   // Might be slow?
   //glTexParameterf( GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR_MIPMAP_LINEAR);
   //glTexParameterf( GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR_MIPMAP_LINEAR);

   glAlphaFunc(GL_GREATER, 0.1f);
}

void load() {
   texSize = nextPowerOfTwo(std::max(gameSettings->fullscreenGW, gameSettings->fullscreenGH));

   //loading textures
   Texture::Add(texSize, texSize, "depthTex", true);
   Texture::Add(texSize, texSize, "bloomTex");
   Texture::Add(texSize, texSize, "fboTex");
   Texture::Add(texSize, texSize, "hblurTex");
   Texture::Add(texSize, texSize, "overlayTex");
   Texture::Add(texSize, texSize, "albedoTex");
   Texture::Add(texSize, texSize, "normalTex");
   Texture::Add(texSize, texSize, "noLightTex");
   Texture::Add(texSize, texSize, "hudTex");
   Texture::Add("Images/Logo.png", "MainLogo");
   Texture::Add("Images/StoreLogo.png", "StoreLogo");
   Texture::Add("Images/AsteroidExplosion.png", "AsteroidExplosion");
   Texture::Add("Images/particle3.png", "Particle");
   Texture::Add("Images/starsdark.bmp", "starsdark.png");
   Texture::Add("Images/AsteroidSurface.png", "AsteroidSurface");
   Texture::Add("Images/Shield.png", "ShieldIcon");
   Texture::Add("Images/ShotIcon.png", "ShotIcon");
   Texture::Add("Images/zoe2.png", "ZoeRedEyes");
   Texture::Add("Images/Weaponbar1.png", "weaponbarbackgroundHoriz");
   Texture::Add("Images/WeaponBarBackground3.png", "weaponbarbackgroundHoriz2");
   Texture::Add("Images/Weaponbar1Vertical.png", "weaponbarbackgroundVert");
   Texture::Add("Images/Weaponbar1Vertical2.png", "weaponbarbackgroundVert2");

   // Weapon Icons
   Texture::Add("Images/weaponicons/blaster.png", "BlasterIcon");
   Texture::Add("Images/weaponicons/tractorbeam.png", "TractorBeamIcon");
   Texture::Add("Images/weaponicons/chargecannon.png", "ChargeCannonIcon");
   Texture::Add("Images/weaponicons/minelayer.png", "MineLayerIcon");
   Texture::Add("Images/weaponicons/pikachuswrath.png", "PikachusWrathIcon");
   Texture::Add("Images/weaponicons/railgun.png", "RailGunIcon");
   Texture::Add("Images/weaponicons/remotebomb.png", "RemoteBombIcon");
   Texture::Add("Images/weaponicons/MissileIcon.png", "MissileIcon");
   Texture::Add("Images/weaponicons/boostershield.png", "RamIcon");

   
   Image::Add(Point3D(0.0, 0.3, -1.0), Point3D(0.3625, 0.1, 1.0), Texture::getTexture("StoreLogo"), "StoreMenuLogo");
   Image::Add(Point3D(0.0, 0.25, -1.0), Point3D(0.3625, 0.1, 1.0), Texture::getTexture("MainLogo"), "MainMenuLogo");

   Particle::initDisplayList();

   // Initialize the framebuffer objects
   initFbo();

   //load the shader files
   elecShader = setShaders( (char *) "./Shaders/elec.vert", (char *) "./Shaders/elec.frag");
   ramShader = setShaders( (char *) "./Shaders/ram.vert", (char *) "./Shaders/ram.frag");
   tractorBeamShader = setShaders( (char *) "./Shaders/toon.vert", (char *) "./Shaders/toon.frag");
   fadeShader = setShaders( (char *) "./Shaders/fade.vert", (char *) "./Shaders/fade.frag");
   tractorFade = setShaders( (char *) "./Shaders/tractorFade.vert", (char *) "./Shaders/tractorFade.frag");
   hBlurShader = setShaders( (char *) "./Shaders/gauss.vert", (char *) "./Shaders/hblur.frag");
   vBlurShader = setShaders( (char *) "./Shaders/gauss.vert", (char *) "./Shaders/vblur.frag");
   billboardShader = setShaders( (char *) "./Shaders/billboard.vert", (char *) "./Shaders/billboard.frag");
   shipXShader = setShaders( (char *) "./Shaders/ship.vert", (char *) "./Shaders/ship.frag");
   shipYShader = setShaders( (char *) "./Shaders/ship.vert", (char *) "./Shaders/shipY.frag");
   backShader = setShaders( (char *) "./Shaders/ship.vert", (char *) "./Shaders/backship.frag");
   hitShader = setShaders( (char *) "./Shaders/shipHit.vert", (char *) "./Shaders/shipHit.frag");
   explosionShader = setShaders( (char *) "./Shaders/bombExplosion.vert", (char *) "./Shaders/bombExplosion.frag");
   ringShader = setShaders( (char *) "./Shaders/ringExplosion.vert", (char *) "./Shaders/ringExplosion.frag");
   gBufferShader = setShaders( (char *) "./Shaders/gbuffer.vert", (char *) "./Shaders/gbuffer.frag");
   lineShader = setShaders( (char *) "./Shaders/gbuffer.vert", (char *) "./Shaders/line.frag");
   bonerShader = setShaders( (char *) "./Shaders/gbuffer.vert", (char *) "./Shaders/boner.frag");
   deferShader = setShaders( (char *) "./Shaders/defer.vert", (char *) "./Shaders/defer.frag");
   timeBombShader = setShaders( (char *) "./Shaders/timeBombExplosion.vert", (char *) "./Shaders/timeBombExplosion.frag");
   
   // Initialize drawBloom to 1.
   GLint drawBloomLoc = glGetUniformLocation(gBufferShader, "drawBloom");
   glUniform1i(drawBloomLoc, 1);

   //load and start BGM
   SoundEffect::Add("Sounds/8-bit3.ogg","8-bit3.ogg", true);
   SoundEffect::Add("Sounds/Asteroids2.ogg", "Asteroids2.ogg", true);
   SoundEffect::Add("Sounds/AsteroidsLevel2.ogg", "AsteroidsLevel2.ogg", true);
   SoundEffect::Add("Sounds/dingding-Asteroids.ogg", "dingding", true);
   SoundEffect::Add("Sounds/Careless_Whisper.ogg","Careless_Whisper.ogg", true);
   SoundEffect::Add("Sounds/AsteroidsMenu2.ogg","AsteroidsMenu2", true);
   SoundEffect::playMusic("AsteroidsMenu2");

   //load sound effects
   //SoundEffect::Add("Sounds/blaster1.wav");
   SoundEffect::Add("Sounds/blaster2.wav", "blaster2.wav");
   SoundEffect::Add("Sounds/BlasterShot2.wav", "BlasterShot2.wav");
   //SoundEffect::Add("Sounds/railgun1.wav");
   SoundEffect::Add("Sounds/Rail2.wav", "Rail2.wav");
   SoundEffect::Add("Sounds/Explosion1.wav", "Explosion1.wav");
   SoundEffect::Add("Sounds/CrystalCollect.wav", "CrystalCollect.wav");
   SoundEffect::Add("Sounds/TractorBeam.wav", "TractorBeam.wav");
   SoundEffect::Add("Sounds/Pikachu.wav", "Pikachu.wav");
   SoundEffect::Add("Sounds/ElectricitySound.wav", "ElectricitySound");
   SoundEffect::Add("Sounds/ShipEngine.wav", "ShipEngine.wav");
   SoundEffect::Add("Sounds/GameOver.wav", "GameOver.wav");
   SoundEffect::Add("Sounds/ShipHit.wav", "ShipHit.wav");
   SoundEffect::Add("Sounds/BlasterHit.wav", "BlasterHit.wav");
   SoundEffect::Add("Sounds/Ram.wav", "Ram");
   SoundEffect::Add("Sounds/CrystalRelease.wav", "CrystalRelease");
   SoundEffect::Add("Sounds/DoubleCrystalRelease.wav", "DoubleCrystalRelease");
   SoundEffect::Add("Sounds/ChargeShotCharge.ogg", "ChargeShotCharge");
   SoundEffect::Add("Sounds/ChargeShotLoop.ogg", "ChargeShotLoop");
   SoundEffect::Add("Sounds/ChargeShotFire.wav", "ChargeShotFire");
   SoundEffect::Add("Sounds/TimedBombExplosion.wav", "TimedBombExplosion");
   SoundEffect::Add("Sounds/BankShard.wav", "ShardBank");
   SoundEffect::Add("Sounds/AsteroidsRocketLaunch.wav", "MissileLaunch");
   SoundEffect::Add("Sounds/BlasterShotWallBounce.wav", "BlasterShotWallBounce");
   SoundEffect::Add("Sounds/BlasterDrone.wav", "BlasterDrone");
   SoundEffect::Add("Sounds/BlasterEnd.wav", "BlasterEnd");
   SoundEffect::Add("Sounds/TimedBombSiren.wav", "TimedBombSiren");
   std::cout << "Load: finished" << std::endl;
}

int main(int argc, char* argv[]) {
   GameStateMode _gsm;
   bool badArugment = false;

   // First check if we're on OSX and if we need to cd up a dir.
#ifdef __APPLE__
   char* dn = dirname(argv[0]);
   std::string bn = basename(dn);
   if (bn.compare("MacOS") == 0) {
      // Go a dir up.
      chdir(dirname(dn));
   }

#endif

   // Decide what mode we were started in.
   if (argc == 1) {
      _gsm = SingleMode;
   } else if (argc == 2) {
      std::string s(argv[1]);
      if (s.compare("-s") == 0) {
         _gsm = ServerMode;
         enableUI = false; // Defaults to true.
         new (&debugoutput) std::ofstream("serverdebug.txt");
         /*
         std::istringstream iss(argv[2]);
         iss >> portNumber;
         */
      } else {
         badArugment = true;
      }
   } else if (argc == 3) {
      std::string s(argv[1]);
      if (s.compare("-c") == 0) {
         _gsm = ClientMode;
         new (&debugoutput) std::ofstream("clientdebug.txt");
         std::string tempIP(argv[2]);
         ipAddress = tempIP;
         /*
         std::string tempPort(argv[3]);
         portNumber = tempPort;
         */
      } else {
         badArugment = true;
      }
   } else {
      badArugment = true;
   }

   if (badArugment) {
      std::cerr << "Usage: AsteroidBlaster [-s | -c <ip>]" << std::endl;
      return 1;
   }
   
   // Seed random numbers.
   srand((unsigned)time(NULL));

   // Initialize networking.
   initNetworking();
   atexit(deinitNetworking);

   // This sets up defaults and reads in any necessary settings.
   gameSettings = new GameSettings();

   if (_gsm == ServerMode) {
      gameSettings->soundOn = false;
      gameSettings->musicOn = false;
   }

   // Initialize timing.
   updateStartTime();
   updateDoubleTime();

   lastUpdateTime = doubleTime();

   // the time difference since last update call
   double timeDiff;

   // Initialize GL/SDL/glew/GLSL related things
   if (enableUI) {
      init();

      //get the quadradic up
      quadric = gluNewQuadric();
      //set the quadradic up
      gluQuadricNormals(quadric, GLU_SMOOTH);
      gluQuadricTexture(quadric, GL_TRUE); // Create Texture Coords


      // Load the textures, sounds, and music.
      load();
   }
   
   // Initialize the gameState
   gameState = new GameState(_gsm);
   
   if (enableUI) {
      // Initialize the screens
      gameState->addScreens();
   }
   
   // Initialize the menus
   mainMenu = new MainMenu(gameState);
   storeMenu = new StoreMenu(gameState);
   // We should change this to pass in gameSettings when something like that exists.
   settingsMenu = new SettingsMenu(gameState);
   helpMenu = new HelpMenu();
   creditsMenu = new CreditsMenu();
   chat = new Input(gameState);
   ipInput = new Input(gameState);
   highScoreList = new HighScoreList();

   if (_gsm == ClientMode) {
      gameState->connect((char*) ipAddress.c_str());
   }
   //turn the menu on for the inial menu display
   if (_gsm == ServerMode || _gsm == ClientMode) {
      mainMenu->menuActive = false;
      gameState->reset();
   } else {
      mainMenu->menuActive = true;
   }

   //Initialize the input manager
   inputManager = new InputManager();
   //Connect the input manager to the gameState
   inputManager->addReceiver(gameState);
   inputManager->addReceiver(mainMenu);
   inputManager->addReceiver(storeMenu);
   inputManager->addReceiver(settingsMenu);
   inputManager->addReceiver(helpMenu);
   inputManager->addReceiver(creditsMenu);
   inputManager->addReceiver(chat);
   inputManager->addReceiver(ipInput);

   //declare the event that will be reused
   SDL_Event* event = NULL;
   if (enableUI) {
      event = new SDL_Event();
   }

   updateDoubleTime();
   timeDiff = doubleTime() - lastUpdateTime;
   lastUpdateTime = doubleTime();

   unsigned long frameLength = 50; // ms = 20 fps
   
   if (_gsm != ServerMode) {
      frameLength = 16; // ms = 60 fps
   }
   long nextFrameDelay = 0;
   long framesAtThisFrameRate = 0;
   long missedFrames = 0;
   unsigned long startTick = 0;

   while (running) {
      if (gameState->gsm == ServerMode) {
         frameLength = 50;
      }
      startTick = SDL_GetTicks();
      
      updateDoubleTime();
      timeDiff = doubleTime() - lastUpdateTime;
      lastUpdateTime = doubleTime();
      
      drawStereo_enabled = false;
      if (gameSettings->drawDeferred) {
         clearAllBuffers();
      }

      if (mainMenu->menuActive) {
         SDL_ShowCursor(SDL_ENABLE);
         mainMenu->update(timeDiff);
         
         if (gameSettings->drawStereo) {
            drawStereo_enabled = true;
            // This requires drawStereo_enabled.
            stereo_eye_left = true;
            mainMenu->draw();
            stereo_eye_left = false;
            mainMenu->draw();
         } else {
            mainMenu->draw();
         }

      } else if (storeMenu->menuActive) {
         SDL_ShowCursor(SDL_ENABLE);
         storeMenu->update(timeDiff);
         gameState->gameIsRunning = false;

         if (gameSettings->drawStereo) {
            drawStereo_enabled = true;
            // This requires drawStereo_enabled.
            stereo_eye_left = true;
            storeMenu->draw();
            if (gameSettings->useOverlay) {
               gameState->draw();
            }

            stereo_eye_left = false;
            storeMenu->draw();
            if (gameSettings->useOverlay) {
               gameState->draw();
            }
         } else {
            storeMenu->draw();
            if (gameSettings->useOverlay) {
               gameState->draw();
            }
         }

      } else if (settingsMenu->menuActive) {
         SDL_ShowCursor(SDL_ENABLE);
         settingsMenu->draw();

      } else if (helpMenu->menuActive) {
         SDL_ShowCursor(SDL_ENABLE);
         helpMenu->draw();

      } else if (creditsMenu->menuActive) {
         SDL_ShowCursor(SDL_ENABLE);
         creditsMenu->update(lastUpdateTime);

      } else {
         gameState->gameIsRunning = true;
         gameState->update(timeDiff);
         lastUpdateTime = doubleTime();
         if (enableUI) {
            if (gameSettings->drawStereo) {
               drawStereo_enabled = true;
               stereo_eye_left = true;
               gameState->draw();
               stereo_eye_left = false;
               gameState->draw();
            } else {
               gameState->draw();
            }
         }
      }

      if (enableUI) {
         while (SDL_PollEvent(event)) {
            inputManager->update(*event);
         }
      }

      nextFrameDelay = startTick + frameLength - SDL_GetTicks();
      if (_gsm == ServerMode) {
         if (nextFrameDelay > 0) {
            SDL_Delay(nextFrameDelay);
         }
      } else {
         // We use -2 here to give some leeway.
         if (nextFrameDelay >= -2) {

            framesAtThisFrameRate++;
            if (framesAtThisFrameRate > 60) {
               missedFrames = 0;
            }

            if (nextFrameDelay > 16 && framesAtThisFrameRate > 60) {
               // Increase speed.
               frameLength = 16; // 60 fps.
               framesAtThisFrameRate = 0;
               missedFrames = 0;
            }

            if (nextFrameDelay > 0)
               SDL_Delay(nextFrameDelay);
         } else {
            missedFrames++;
            framesAtThisFrameRate = 0;
            if (missedFrames > 20) {
               if (frameLength == 16) {
                  // Decrease speed.
                  frameLength = 32; // 30 fps.
                  missedFrames = 0;
               }
            }
         }
      }
   }

   // Closing up!
   if (gameState->gsm == ClientMode) {
      gameState->disconnect();
   }

   glDeleteFramebuffersEXT(1, &fbo);
   glDeleteRenderbuffersEXT(1, &depthbuffer);

   if (gameSettings->fullscreen) {
      SDL_WM_ToggleFullScreen(gDrawSurface);
      //SDL_SetVideoMode(0, 0, 0, SDL_OPENGL);
   }

   if (gameState->gsm != ServerMode) {
      gameSettings->writeOut();
   }

   // Clean up
   delete gameState;
   delete mainMenu;
   delete storeMenu;
   delete settingsMenu;
   delete helpMenu;
   delete creditsMenu;
   delete inputManager;
   delete chat;
   delete ipInput;
   delete highScoreList;
   SoundEffect::FreeAll();

   return 0;
}
