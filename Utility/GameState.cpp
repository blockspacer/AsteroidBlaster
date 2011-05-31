/**
 * GameState - Stores the state of the game.
 * Lists of objects, etc.
 * @author Sterling Hirsh
 * @date 1-23-11
 * CPE 476
 */

#include "Utility/GameState.h"

#include "Utility/Matrix4.h"
#include "Utility/Music.h"
#include "Utility/SoundEffect.h"
#include "Utility/Collision.h"
#include "Utility/GlobalUtility.h"

#include "Graphics/Texture.h"
#include "Graphics/Skybox.h"
#include "Graphics/Sprite.h"
#include "Graphics/Camera.h"

#include "Particles/Particle.h"

#include "Text/Text.h"
#include "Text/Input.h"
#include "Text/GameMessage.h"

#include "Items/AsteroidShip.h"
#include "Items/BoundingSpace.h"
#include "Items/Asteroid3D.h"
#include "Items/Spring.h"

#include "Menus/MainMenu.h"
#include "Menus/StoreMenu.h"

#include "HUD/ProgressBar.h"
#include "HUD/WeaponDisplay.h"
#include "HUD/Minimap.h"
#include "HUD/Screen.h"

#include "Network/UDP_Server.h"
#include "Network/UDP_Client.h"
#include "Network/NetTimer.h"
#include "Network/NetShard.h"
#include "Network/NetAsteroid.h"
#include "Network/NetShip.h"

#include <math.h>
#include <stdio.h>
#include <fstream>
#include <sstream>
#include <boost/thread.hpp>
#include <boost/archive/text_oarchive.hpp>
#include <boost/archive/text_iarchive.hpp>

#define SAVEFILENAME "AsteroidBlaster.sav"

#ifndef M_PI
#define M_PI           3.14159265358979323846
#endif

extern double minimapSizeFactor;

std::ostringstream sstream2; // TODO: Better name plz.

GameState::GameState(GameStateMode _gsm) :
   custodian(this) {
      gsm = _gsm;
      if (gsm == SingleMode) {
         io = NULL;
         udpClient = NULL;
         udpServer = NULL;
         networkThread = NULL;
      } else if (gsm == MenuMode) {
         io = NULL;
         udpClient = NULL;
         udpServer = NULL;
         networkThread = NULL;
      } else if (gsm == ClientMode) {
         io = new boost::asio::io_service();
         udpClient = new UDP_Client(*io, this, ipAddress, portNumber);
         udpServer = NULL;
         networkThread = new boost::thread(boost::bind(&boost::asio::io_service::run, io));
      } else if (gsm == ServerMode) {
         io = new boost::asio::io_service();
         udpClient = NULL;

         std::istringstream iss(portNumber);
         int tempPortNumber;
         iss >> tempPortNumber;
         udpServer = new UDP_Server(*io, this, tempPortNumber);
         networkThread = new boost::thread(boost::bind(&boost::asio::io_service::run, io));
      }

      godMode = false;
      gameIsRunning = true;
      levelOver = false;
      if (gsm == ServerMode) {
         usingShipCamera = false;
      } else {
         usingShipCamera = true;
      }

      /* A view frustum culled list of objects to be used for drawing and by
         the shooting AI.
         */
      viewFrustumObjects = NULL;
      targetableViewFrustumObjects = NULL;

      worldSize = WORLD_SIZE;
      skybox = new Skybox();
      if (gsm == ClientMode) {
         while (udpClient->shipID == -1) {
            std::cout << "udpClient->shipID == -1" << std::endl;
            SDL_Delay(1);
         }
         std::cout << "udpClient->shipID != -1" << std::endl;

         Object3D* temp = NULL;

         // Wait until we have a valid ship object from the server.
         while((temp = custodian[udpClient->shipID]) == NULL) {
            SDL_Delay(1);
         }

         ship = dynamic_cast<AsteroidShip*>(temp);
         if (ship == NULL) {
            std::cout << "GameState::setShipWithID came up with NULL" << std::endl;
            exit(1);
         } else {
            std::cout << "GameState::reset on client successful! with ship id of " << ship->id << std::endl;
         }
      } else {
         ship = new AsteroidShip(this);
      }

      clientCommand.shipID = ship->id;
      
      spring = new Spring(this);
      minimap = new Minimap(ship);

      shipCamera = new Camera(ship);
      spring->attach(ship, shipCamera);
      spectatorCamera = new Camera(false);
      //make it look at the center of the map for spectator mode
      spectatorCamera->lookAt(0.0, 0.0, 0.0);
      spectatorSpeed = 0.2;
      spectatorRadius = 120.0;



      cube = new BoundingSpace(worldSize / 2, 0, 0, 0, this);
      //sphere = new BoundingSphere(worldSize, 0, 0, 0);
      // Set up our text objects to be displayed on screen.
      curFPS = 0;

      // Init Text Objects
      SDL_Rect position = {0,0};
      std::string fontName = DEFAULT_FONT;

      //new all the text class we will be using
      FPSText = new Text("FPS: ", curFPS, "",  hudFont, position);
      scoreText = new Text("Score: ", ship->getScore(), "",  hudFont, position);
      shardText = new Text("Shards: ", ship->getShards(), "",  hudFont, position);
      bankedShardText = new Text("Banked Shards: ", ship->bankedShards, "",  hudFont, position);
      unbankedShardText = new Text("Unbanked Shards: ", ship->unbankedShards, "",  hudFont, position);
      //ammoText = new Text("Ammo: ", sstream2.str(), "",  hudFont, position);
      //weaponText = new Text("Current Weapon: ", ship->getCurrentWeapon()->getName(), "",  hudFont, position);
      timerText = new Text("", sstream2.str(), "", hudFont, position);
      timerText->alignment = RIGHT_ALIGN;
      curLevelText = new Text("Level: ", curLevel, "",  hudFont, position);
      curLevelText->alignment = RIGHT_ALIGN;

      // Clear the sstream2
      sstream2.str("");

      // Improve the positioning code.
      weaponReadyBar = new ProgressBar(0.75f, 0.05f, -1.2f, -0.3f);
      
      shardBankBar = new ProgressBar(0.06f, 0.4f, -1.1f, p2wx(10));
      shardBankBar->setHorizontal(true);

      float healthSpan = (float)gameSettings->GW / (float)gameSettings->GH *
         2.0f * 0.6f;
      const float healthHeight = 0.10f;
      const float weaponBarHeight = 0.3f;
      healthBar = new ProgressBar(healthHeight, healthSpan, 0.0f, 0.95f - (healthHeight * 0.5f));
      healthBar->setHorizontal(true);
      healthBar->setSkew(0.0, 0.05f);
      //weaponBar = new WeaponDisplay(healthHeight * 1.5f, 1.0f, 0.0f, -0.9f + (healthHeight * 0.5f), this);
      weaponBar = new WeaponDisplay(weaponBarHeight, weaponBarHeight, 0.0f, -0.9f + (weaponBarHeight * 0.5f), this);
      //healthBar->setIcon("ShieldIcon");
      weaponReadyBar->setIcon("ShotIcon");

      // Start off at level 1.
      curLevel = 1;
      // Spawn one more asteroid for each level.
      numAsteroidsToSpawn = curLevel;

      if (gsm != ClientMode && gsm != ServerMode) {
         // Set up objects.
         custodian.add(ship);
         // It crashes without this. :/
         initAsteroids();
      } else {
         custodian.update();
      }
      

      doYaw = 0;
      mouseX = 0;
      mouseY = 0;

      // Make a good formula here for how many seconds a level should last.
      levelDuration = 180;

      godMode = false;

      // TODO: comment this or rename it.
      isW = isA = isS = isD = false;

      drawGraphics = true;

   }

/**
 * Deconstructor: clean up all of our objects.
 */
GameState::~GameState() {
   delete skybox;
   delete ship;
   delete shipCamera;
   delete spectatorCamera;
   delete spring;
   delete cube;
   delete weaponReadyBar;
   delete shardBankBar;
   if (udpServer != NULL) {
      delete udpServer;
   }
   if (udpClient != NULL) {
      delete udpClient;
   }
   // Somehow makes it segfault when quitting so commented out
   /*if (io != NULL) {
      io->stop();
      delete io;
   }*/
}

/**
 * The game should run for levelDuration # of seconds.
 */
void GameState::setLevelTimer() {
   levelTimer.setCountDown(levelDuration);
   // levelTimer is implicltly now 'active'.
}

/**
 * Pause the levelTimer. It should remain at its current time left when it
 * is resumed again in the future.
 */
void GameState::pauseLevelTimer() {
   levelTimer.pause();
}

/**
 * Resume the levelTimer.
 */
void GameState::resumeLevelTimer() {
   levelTimer.resume();
}

void GameState::addAIPlayer() {
   AsteroidShip* otherShip = new AsteroidShip(this);
   double randX = (randdouble())*(worldSize / 2);
   double randY = (randdouble())*(worldSize / 2);
   double randZ = (randdouble())*(worldSize / 2);
   otherShip->position->update(randX, randY, randZ);
   otherShip->flyingAI->enable();
   otherShip->shooter->enable();
   custodian.add(otherShip);
}

void GameState::addNetworkPlayer(unsigned clientID) {
   AsteroidShip* otherShip = new AsteroidShip(this);
   double randX = (randdouble())*(worldSize / 2);
   double randY = (randdouble())*(worldSize / 2);
   double randZ = (randdouble())*(worldSize / 2);
   otherShip->position->update(randX, randY, randZ);
   custodian.add(otherShip);
   custodian.shipsByClientID.insert(std::pair<unsigned, AsteroidShip*>(clientID, otherShip));
}

/**
 * Initialize the displays once the textures have been loaded.
 */
void GameState::addScreens() {
   printf("screens added\n");
   screens.push_back(Screen(0, 0, Texture::getTexture("hblurTex")));
   screens.push_back(Screen(0, 1, Texture::getTexture("bloomTex")));
   screens.push_back(Screen(1, 0, Texture::getTexture("fboTex")));
   screens.push_back(Screen(0, 2, Texture::getTexture("overlayTex")));
}

/**
 * This is the step function.
 */
void GameState::update(double timeDiff) {

   // if it's not in MenuMode
   if (gsm != MenuMode) {
      // If the level should be over, let's go to the store menu.
      if((levelTimer.isRunning && levelTimer.getTimeLeft() <= 0) || 
               (gameIsRunning && 
               (gsm == SingleMode) && 
               custodian.asteroidCount == 0 && 
               custodian.shardCount == 0) ) {
         nextLevel();
         return;
      }

      // the last 6 seconds of the stage
      if (levelTimer.getTimeLeft() <= 6) {
         // Check if it is done waiting until going to the next level
         std::ostringstream gameMsg;
         gameMsg << "Next Level In " << (int)levelTimer.getTimeLeft();
         GameMessage::Add(gameMsg.str(), 30, 0);
      }
   }


   std::vector<Object3D*>* objects = custodian.getListOfObjects();
   std::set<CollisionBase*, compareByDistance>* collisions;
   std::set<CollisionBase*, compareByDistance>::iterator curCollision;

   // Keep items in the box.
   // cube->constrain(ship); why did we do this twice?

   if (!ship->flyingAI->isEnabled() && !ship->shooter->isEnabled()) {
      ship->readCommand(clientCommand);
   }

   // Update each item.
   for (item = objects->begin(); item != objects->end(); ++item) {
      if (*item == NULL)
         continue;
      (*item)->update(timeDiff);
      cube->constrain(*item);
   }

   // Add items that should be added, remove items that should be removed, update
   // positions in the sorted lists.
   custodian.update();

   // Get updated list.
   objects = custodian.getListOfObjects();

   Particle::updateParticles(timeDiff);
   Sprite::updateSprites(timeDiff);
   MeshFace::updateIndependentFaces(timeDiff);
   GameMessage::updateAllMessages(timeDiff);

   // Check every object for collisions.
   for (item = objects->begin(); item != objects->end(); ++item) {
      collisions = custodian.findCollisions(*item, false);
      for (curCollision = collisions->begin(); curCollision != collisions->end(); ++curCollision) {
         (*curCollision)->handleCollision();
      }
      delete collisions;
      // TODO: fix memory leak.
   }

   SoundEffect::updatePositions(ship);

   // Update all of the text seen on screen.
   updateText();

   weaponReadyBar->setAmount(ship->getCurrentWeaponCoolDown());
   shardBankBar->setAmount(ship->bankTimer.isRunning ? ship->bankTimer.getAmountComplete() : 0);
   healthBar->setAmount(((float) ship->health / (float) ship->healthMax));
   cube->update(timeDiff);
   minimap->update(timeDiff);
   spring->update(timeDiff);
   spectatorCameraUpdate(timeDiff);
   ++curFrame;
}

void GameState::networkUpdate(double timeDiff) {
   // CLIENT-->SERVER stuff~~~~~~~~~~~~~~~~~~~~~~
   if (gsm == ClientMode) {
      // For sending clientcommands to the server
      static double tempClientSend = 0;
      tempClientSend += timeDiff;
      if (tempClientSend >= 0.05) {
         std::ostringstream oss;
         boost::archive::text_oarchive oa(oss);
         int i = NET_CLIENTCOMMAND;
         oa << i << (const ClientCommand)clientCommand;
         udpClient->send(oss.str(), udpClient->serverEndPoint);
         tempClientSend = 0;
      }

      
   // SERVER-->CLIENT stuff~~~~~~~~~~~~~~~~~~~~~~
   } else if (gsm == ServerMode){
      static double clientCommandTime = 0;
      static double asteroidTime = 0;
      static double shardTime = 0;
      static double gameStat = 0;

      clientCommandTime += timeDiff;
      asteroidTime += timeDiff;
      shardTime += timeDiff;
      gameStat += timeDiff;


      if (clientCommandTime >= 0.05) {
         std::set<AsteroidShip*>::iterator iter = custodian.ships.begin();
         for (;iter != custodian.ships.end();iter++) {
            NetShip testNetship;
            testNetship.fromObject(*iter);
            std::ostringstream oss;
            boost::archive::text_oarchive oa(oss);
            int i = NET_OBJ_SHIP;
            oa << i << testNetship;
            udpServer->sendAll(oss.str());
         }
         clientCommandTime = 0;
      }

      if (asteroidTime >= 0.1) {
         std::set<Asteroid3D*>::iterator iter = custodian.asteroids.begin();
         for (;iter != custodian.asteroids.end();iter++) {
            NetAsteroid testNetAsteroid;
            testNetAsteroid.fromObject(*iter);
            std::ostringstream oss;
            boost::archive::text_oarchive oa(oss);
            int i = NET_OBJ_ASTEROID;
            oa << i << testNetAsteroid;
            udpServer->sendAll(oss.str());
         }
         asteroidTime = 0;
      }

      if (shardTime >= 0.1) {
         std::set<Shard*>::iterator iter = custodian.shards.begin();
         for (;iter != custodian.shards.end();iter++) {
            NetShard testNetShard;
            testNetShard.fromObject(*iter);
            std::ostringstream oss;
            boost::archive::text_oarchive oa(oss);
            int i = NET_OBJ_SHARD;
            oa << i << testNetShard;
            udpServer->sendAll(oss.str());
         }
         shardTime = 0;
      }

      if (gameStat >= 3.0) {
         std::ostringstream oss;
         boost::archive::text_oarchive oa(oss);
         int i = NET_LEVEL_UPDATE;
         double timeLeft = levelTimer.getTimeLeft();
         oa << i << timeLeft << curLevel;
         udpServer->sendAll(oss.str());
         gameStat = 0;
      }
   }
}

void GameState::spectatorCameraUpdate(double timeDiff) {
   static double pieCounter = M_PI*0.75;
   if (pieCounter >= 2*M_PI) { pieCounter = 0; }
   spectatorCamera->position->x = sin(pieCounter)*spectatorRadius;
   spectatorCamera->position->y = sin(pieCounter)*(spectatorRadius/3);
   spectatorCamera->position->z = -cos(pieCounter)*spectatorRadius;
   spectatorCamera->lookAt(0.0,0.0,0.0);
   pieCounter += timeDiff*spectatorSpeed;
}

/**
 * Draw objects in the minimap.
 */
void GameState::drawMinimap() {
   if ((gsm != MenuMode) && !ship->isRespawning())
      minimap->draw();
}

/**
 * Draws all Displays.
 */
void GameState::drawScreens() {
   if (!showBloomScreen) { return; }
   for (unsigned i = 0; i < screens.size(); i++) {
      screens[i].draw();
   }
}

/**
 * Returns whether the minimap is on.
 */
bool GameState::minimapOn() {
   return minimap->isEnabled();
}

/**
 * Toggles the minimap on or off.
 */
void GameState::toggleMinimap() {
   minimap->toggle();
}

/**
 * Draw everything in gamestate
 */
void  GameState::draw() {
   if (!drawGraphics) {
      glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
      SDL_GL_SwapBuffers();
      return;
   }
   // the time difference since last update call
   double timeDiff = doubleTime() - lastDrawTime;

   setCurFPS(1 / timeDiff);

   // Clear the screen
   glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
   glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
   usePerspective();

   glMatrixMode(GL_MODELVIEW);


   // Draw glowing objects to a texture for the bloom effect.
   aspect = (float)gameSettings->GW/(float)gameSettings->GH;
   if (gameSettings->bloom) {
      glBindFramebufferEXT(GL_FRAMEBUFFER_EXT, fbo);
      glDrawBuffer(GL_COLOR_ATTACHMENT0_EXT);
      glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
      glPushMatrix();

      if (gsm == MenuMode) {
         gluLookAt(0, 0, 20, // Eye at origin + 20.
               0, 0, -1, // X goes right, Z goes away.
               0, 1, 0); // Y goes up.
      }

      drawObjects(true);

      glPopMatrix();
      glBindFramebufferEXT(GL_FRAMEBUFFER_EXT, 0);

      hBlur();
      vBlur();

      glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
   }

   usePerspective();

   // Draw the main screen
   glPushMatrix();
      if (gsm == MenuMode) {
         gluLookAt(0, 0, 20, // Eye at origin.
               0, 0, -1, // X goes right, Z goes away.
               0, 1, 0); // Y goes up.
      }
      drawObjects();
   glPopMatrix();

   glPushMatrix();
      // Draw the hud
      glClear(GL_DEPTH_BUFFER_BIT);
      if (gameSettings->bloom) {
         drawBloom();
      }

      if (gsm != MenuMode) {
         drawHud();
      }
   glPopMatrix();

   if (gameSettings->useOverlay) {
      drawOverlay();
   }

   drawScreens();

   // TODO: Make this clearer. Why not swap when lookAt is true?
   if (gsm != MenuMode) {
      SDL_GL_SwapBuffers();
   }
   lastDrawTime = doubleTime();
}

/**
 * Draw objects
 */
void GameState::drawObjects(bool drawGlow) {
   if (!drawGraphics) {
      return;
   }

   // This decides where the camera is, either ship or spectator camera.
   Camera *currentCamera;
   if (usingShipCamera && gsm != ServerMode) {
      currentCamera = shipCamera;
      shipCamera->setViewVector(ship->getViewVector());
      shipCamera->setOffset(*ship->getCameraOffset());
   } else {
      currentCamera = spectatorCamera;
   }

   // If this gameState is not mainGamestate menu, set the camera and shake.
   if (gsm != MenuMode) {
      currentCamera->setCamera(true);
      shipCamera->shake(ship->getShakeAmount());
   }
   

   // Draw the skybox if this gameState is not mainmenu and is glow shading
   if ((gsm != MenuMode) && !drawGlow) {
      skybox->draw(currentCamera);
   }

   // Draw the outer cube
   cube->draw();

   // Get a list of all of the objects after culling them down to the view frustum.
   viewFrustumObjects = ship->getRadar()->getViewFrustumReading();

   if (!drawGlow) {
      glBlendFunc(GL_SRC_ALPHA, GL_ZERO);
      if ((gsm != MenuMode) && usingShipCamera && gsm != ServerMode)
         ship->drawCrosshair();

      for (listIter = viewFrustumObjects->begin(); listIter != viewFrustumObjects->end(); ++listIter) {
         if (*listIter == NULL) {
            continue;
         } else if (*listIter == ship && (gsm == MenuMode)) {
            // Don't draw the ship in first Person mode.
         } else {
            (*listIter)->draw();
         }
      }
   } else {
      glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);

      for (listIter = viewFrustumObjects->begin(); listIter != viewFrustumObjects->end(); ++listIter) {
         if (*listIter == NULL || *listIter == ship) {
            continue;
         } else {
            (*listIter)->drawGlow();
         }
      }

      if (gsm != MenuMode && gsm != ServerMode) {
         ship->draw();
      }
   }

   delete viewFrustumObjects;
}

void GameState::hBlur() {
   int tex = Texture::getTexture("fboTex");
   GLint texLoc = glGetUniformLocation(hBlurShader, "RTscene");
   glUseProgram(hBlurShader);
   glUniform1i(texLoc, tex);
   
   glBindFramebufferEXT(GL_FRAMEBUFFER_EXT, fbo);
   glDrawBuffer(GL_COLOR_ATTACHMENT1_EXT);
   glClear(GL_COLOR_BUFFER_BIT);

   drawScreenQuad(tex);

   glBindFramebufferEXT(GL_FRAMEBUFFER_EXT, 0);
}

void GameState::vBlur() {
   int tex = Texture::getTexture("hblurTex");
   GLint texLoc = glGetUniformLocation(vBlurShader, "blurSampler");
   glUseProgram(vBlurShader);
   glUniform1i(texLoc, tex);

   glBindFramebufferEXT(GL_FRAMEBUFFER_EXT, fbo);
   glDrawBuffer(GL_COLOR_ATTACHMENT2_EXT);
   glClear(GL_COLOR_BUFFER_BIT);
   
   drawScreenQuad(tex);

   glUseProgram(0);

   glBindFramebufferEXT(GL_FRAMEBUFFER_EXT, 0);
}

void GameState::drawBloom() {
   drawScreenQuad(Texture::getTexture("bloomTex"));
}

// Draw the Hud, which includes, minimap, weapon/health bar, text overlay...
void GameState::drawHud() {
   glPushMatrix();
      useOrtho();

      /* Set the shipCamera using the location of your eye,
       * the location where you're looking at, and the up vector.
       * The shipCamera is set to be just 0.25 behind where you're looking at.
       */
      gluLookAt(0, 0, 0.25, 0, 0, 0, 0, 1, 0);

      //glBlendFunc(GL_ONE_MINUS_SRC_ALPHA, GL_SRC_ALPHA);


      /* We need to disable the lighting temporarily
       * in order to set the color properly.
       */
      glDisable(GL_LIGHTING);
      glDisable(GL_CULL_FACE);
      drawAllText();
      if(usingShipCamera && !ship->isRespawning() && gsm != ServerMode){
         weaponReadyBar->draw();
         shardBankBar->draw();
         healthBar->draw();
         weaponBar->draw();
         drawMinimap();
      }
      usePerspective();
      
      glEnable(GL_CULL_FACE);

   glPopMatrix();
}

void GameState::drawOverlay() {
   glPushMatrix();
      glClear(GL_DEPTH_BUFFER_BIT);
      useOrtho();
      glDisable(GL_LIGHTING);
      float minY = -1.0;
      float maxY = 1.0;
      float minX = -1.0f * aspect;
      float maxX = 1.0f * aspect;

      if (!gameIsRunning) {
         glColor4f(0.0, 1.0, 0.0, 0.7f);
         float scale = 1.0;
         glBlendFunc(GL_ZERO, GL_ONE_MINUS_SRC_ALPHA);
         glBegin(GL_QUADS);
         glVertex3f(minX * scale, minY * scale, 0.0);
         glVertex3f(maxX * scale, minY * scale, 0.0);
         glVertex3f(maxX * scale, maxY * scale, 0.0);
         glVertex3f(minX * scale, maxY * scale, 0.0);
         glEnd();
      }

      int tex = Texture::getTexture("overlayTex");
      drawScreenQuad(tex);
      usePerspective();
   glPopMatrix();
}

/**
 * Draw all of the text in the allTexts list to the screen.
 * This function should be called once per display loop.
 * We do all of the looking at, lighting changes, etc.
 * one time here to improve efficiency.
 */
void GameState::drawAllText() {

   SDL_Rect position;
   const Sint16 positionDifferenceY = 15;


   // Position all the text on left side
   position.x = 10;
   position.y = 30;
   FPSText->setPosition(position);

   position.y = (Sint16) (position.y + positionDifferenceY);
   scoreText->setPosition(position);

   position.y = (Sint16) (position.y + positionDifferenceY);
   shardText->setPosition(position);
   
   position.y = (Sint16) (position.y + positionDifferenceY);
   bankedShardText->setPosition(position);
   
   position.y = (Sint16) (position.y + positionDifferenceY);
   unbankedShardText->setPosition(position);
   
   position.y = (Sint16) (position.y + positionDifferenceY);

   shardBankBar->y = p2wy(position.y);

   // Position all the text on right side
   position.x = (Sint16)(gameSettings->GW - 30);
   position.y = 30;
   curLevelText->setPosition(position);

   position.y = (Sint16) (position.y + positionDifferenceY);
   timerText->setPosition(position);

   // Draw all the text on left side
   FPSText->draw();

   if (gsm != ServerMode) {
      scoreText->draw();
      shardText->draw();
      bankedShardText->draw();
      unbankedShardText->draw();
   }

   // Draw all the text on right side
   timerText->draw();
   curLevelText->draw();

   // Draw all messages
   GameMessage::drawAllMessages();
}

/**
 * Update the values contained in all of the texts.
 */
void GameState::updateText() {
   FPSText->updateBody((int)curFPS);
   scoreText->updateBody(ship->getScore());
   shardText->updateBody(ship->getShards());
   bankedShardText->updateBody(ship->bankedShards);
   unbankedShardText->updateBody(ship->unbankedShards);
   //weaponText->updateBody(ship->getCurrentWeapon()->getName());
   // If the gun has infinite ammo, say so.
   /*if(ship->getCurrentWeapon()->curAmmo == -1)
     ammoText->updateBody("Inf");
     else {
     sstream2 << ship->getCurrentWeapon()->curAmmo;
     ammoText->updateBody(sstream2.str());
     sstream2.str("");
     }*/

   // Update the timer on screen to show the minutes & seconds remaining.
   std::string minutes;
   sstream2 << minutesRemaining(levelTimer.getTimeLeft());
   //sstream2 << minutesRemaining(levelDuration - (doubleTime() - timeStarted));
   minutes = sstream2.str();
   sstream2.str("");
   std::string seconds;
   int secsRemaining = secondsRemainder(levelTimer.getTimeLeft());
   sstream2 << secondsRemainder(levelTimer.getTimeLeft());

   // Throw a 0 in front, so that it comes out 2:09 instead of 2:9.
   if (secsRemaining < 10) {
      seconds = "0" + sstream2.str();
   }
   else {
      seconds = sstream2.str();
   }

   sstream2.str("");
   timerText->updateBody(minutes + ":" + seconds);
   //timerText->updateBody(seconds);
}

void GameState::initAsteroids() {
   /**
    * Don't init asteroids in the menu.
    */
   if (gsm == MenuMode) {
      return;
   }

   Asteroid3D* tempAsteroid;

   std::set<CollisionBase*, compareByDistance>* collisions;
   std::set<CollisionBase*, compareByDistance>::iterator curCollision;

   /* We want this spaceHolder because we don't want to spawn asteroids
    * too close to the ship.
    */
   /* TODO: One of these should go around each ship. */
   Object3D* spaceHolder = new Object3D(this);
   spaceHolder->minX = spaceHolder->minY = spaceHolder->minZ = -10;
   spaceHolder->maxX = spaceHolder->maxY = spaceHolder->maxZ = 10;
   custodian.add(spaceHolder);

   int numCollisions = 0;
   double asteroidSize = 8;
   // Spawn the initial asteroids for the game.
   for (int i = 0; i < numAsteroidsToSpawn; ++i) {
      tempAsteroid = new Asteroid3D(asteroidSize, worldSize, this, true);
      // Add each asteroid to the custodian so we know of its existence.
      custodian.add(tempAsteroid);
      do {
         custodian.update();
         collisions = custodian.findCollisions(tempAsteroid, true);
         numCollisions = (int) collisions->size();
         if (numCollisions > 0) {
            tempAsteroid->newRandomPosition();
         }
         // TODO: Fix memory leak.
         delete collisions;
      } while (numCollisions > 0);
   }
   spaceHolder->shouldRemove = true;
   custodian.update();
}

void GameState::setCurFPS(double fpsIn) {
   curFPS = fpsIn;
}

/**
 * Reset everything in the game to play again
 */
void GameState::reset(bool shouldLoad) {
   delete shipCamera;
   delete spectatorCamera;
   delete cube;
   delete ship;
   delete spring;
   delete minimap;

   custodian.clear();
   Particle::Clear();
   MeshFace::Clear();
   custodian.update();

   gameIsRunning = true;
   // The level is not over when we're starting a new game.
   levelOver = false;

   curLevel = 1;
   //temp
   if(shouldLoad) {
      load();
   }

   numAsteroidsToSpawn = curLevel;
   curLevelText->updateBody(curLevel);

   cube = new BoundingSpace(worldSize / 2, 0, 0, 0, this);

   std::cout << "Resetting." << std::endl;
   
   if (gsm == ClientMode) {
      while (udpClient->shipID == -1) {
         std::cout << "udpClient->shipID == -1" << std::endl;
         SDL_Delay(1);
      }
      std::cout << "udpClient->shipID != -1" << std::endl;

      Object3D* temp = NULL;

      // Wait until we have a valid ship object from the server.
      while((temp = custodian[udpClient->shipID]) == NULL) {
         SDL_Delay(1);
      }

      ship = dynamic_cast<AsteroidShip*>(temp);
      if (ship == NULL) {
         std::cout << "GameState::setShipWithID came up with NULL" << std::endl;
         exit(1);
      } else {
         std::cout << "GameState::reset on client successful! with ship id of " << ship->id << std::endl;
      }
   } else {
      ship = new AsteroidShip(this);
   }

   clientCommand.reset();
   clientCommand.shipID = ship->id;
   spring = new Spring(this);
   cube->constrain(ship);
   minimap = new Minimap(ship);
   shipCamera = new Camera(ship);
   spring->attach(ship, shipCamera);
   spectatorCamera = new Camera(false);


   if (gsm != ClientMode && gsm != ServerMode) {
      custodian.add(ship);
      // Crashes without this.
      initAsteroids();
   } else {
      custodian.update();
   }

   GameMessage::Clear();
   addLevelMessage();

   setLevelTimer();

}

void GameState::addLevelMessage() {
   std::ostringstream levelMessage;
   levelMessage << "Level " << curLevel;
   GameMessage::Add(levelMessage.str(), 30, 5);
}

void GameState::nextLevel() {
   SoundEffect::stopAllSoundEffect();

   std::set<AsteroidShip*>::iterator shipIter;

   for (shipIter = custodian.ships.begin(); shipIter != custodian.ships.end(); ++shipIter) {
      (*shipIter)->atLevelEnd();
   }

   std::set<Asteroid3D*>::iterator asteroidIter;
   
   for (asteroidIter = custodian.asteroids.begin(); asteroidIter != custodian.asteroids.end(); ++asteroidIter) {
      (*asteroidIter)->shouldRemove = true;;
   }
   
   std::set<Shard*>::iterator shardIter;
   
   for (shardIter = custodian.shards.begin(); shardIter != custodian.shards.end(); ++shardIter) {
      (*shardIter)->shouldRemove = true;;
   }

   std::set<Shot*>::iterator shotIter;
   
   for (shotIter = custodian.shots.begin(); shotIter != custodian.shots.end(); ++shotIter) {
      (*shotIter)->shouldRemove = true;;
   }

   if (!ship->shooter->isEnabled() && !ship->flyingAI->isEnabled()) {
      storeMenu->menuActive = true;
      Music::stopMusic();
      Music::playMusic("8-bit3.ogg");
   }

   setLevelTimer();

   minimap = new Minimap(ship);
   gameIsRunning = true;
   // The current level is over when we're advancing to the next level.
   levelOver = true;
   curLevel++;
   curLevelText->updateBody(curLevel);
   numAsteroidsToSpawn = curLevel;
   printf("Level'd up to %d!\n",curLevel);

   //Add AI player
   if (gsm == SingleMode && (curLevel > 1)) {
      addAIPlayer();
   }

   //Add Asteroids
   if (gsm == SingleMode || gsm == ServerMode) {
      initAsteroids();
   }

   // if it's ServerMode, send the next level packet so that
   // clients go to store menu mode
   if (gsm == ServerMode) {
      std::ostringstream oss;
      boost::archive::text_oarchive oa(oss);
      int i = NET_ACTIVATE_STOREMENU;
      oa << i;
      udpServer->sendAll(oss.str());
   }


   custodian.update();
   GameMessage::Clear();
   addLevelMessage();
}

/**
 * Handles the player pressing down a key
 */
void GameState::keyDown(int key, int unicode) {
   //If game is not running, do not take input anymore
   if(!gameIsRunning || chat->chatActive){ return;}

   switch(key) {
      //movement keys, not valid if flying AI is enabled

      if(!ship->flyingAI->isEnabled()) {
      case SDLK_w:
         isW = true;
         if (isS) {
            clientCommand.forwardAcceleration = 0;
         } else {
            clientCommand.forwardAcceleration = 1;
         }
         break;

      case SDLK_s:
         isS = true;
         if (isW) {
            clientCommand.forwardAcceleration = 0;
         } else {
            clientCommand.forwardAcceleration = -1;
         }
         break;

      case SDLK_a:
         isA = true;
         if (isD) {
            clientCommand.yawSpeed = 0;
         } else {
            clientCommand.yawSpeed = 1.0;
         }
         break;

      case SDLK_d:
         isD = true;
         if (isA) {
            clientCommand.yawSpeed = 0.0;
         } else {
            clientCommand.yawSpeed = -1.0;
         }
         break;

      case SDLK_q:
         if (ship->isBarrelRollingLeft < 0 && ship->isBarrelRollingRight < 0) ship->isBarrelRollingLeft = 1;
         //clientCommand.rightAcceleration = -1;
         break;

      case SDLK_e:
         if (ship->isBarrelRollingLeft < 0 && ship->isBarrelRollingRight < 0) ship->isBarrelRollingRight = 1;
         //clientCommand.rightAcceleration = 1;
         break;

      case SDLK_SPACE:
         clientCommand.upAcceleration = 1;
         break;

      case SDLK_LCTRL:
         clientCommand.upAcceleration = -1;
         break;

      case SDLK_RSHIFT:
         doYaw = !doYaw;
         clientCommand.rollSpeed = 0;
         break;

         /*
            case SDLK_LSHIFT:
            ship->setBoost(true);
            break;
            */

      case SDLK_b:
         clientCommand.brake = true;
         break;
      }


      //Camera controls
   case SDLK_8:
      ship->setZoomSpeed(-1);
      break;
   case SDLK_9:
      ship->setZoomSpeed(1);
      break;

   case SDLK_t:
      ship->nextView();
      break;
   case SDLK_y:
      usingShipCamera = !usingShipCamera;
      break;
   case SDLK_KP_PLUS:
      spectatorSpeed += 0.1;
      break;
   case SDLK_KP_MINUS:
      spectatorSpeed -= 0.1;
      break;
   case SDLK_KP_MULTIPLY:
      spectatorRadius += 10.0;
      break;
   case SDLK_KP_DIVIDE:
      spectatorRadius -= 10.0;
      break;


      // AI controls
   case SDLK_g:
      //TODO: Based on how many shooting AIs this ship has, activate the correct one.
      if(ship->shooter->isEnabled())
         ship->shooter->disable();
      else
         ship->shooter->enable();

      break;

   case SDLK_f:
      // When you disable the flyingAI, stop accelerating the ship.
      if(ship->flyingAI->isEnabled()) {
         ship->flyingAI->disable();
         clientCommand.reset();
      } else {
         // Stop accelerating the ship when turning on the flyingAI as well.
         clientCommand.reset();
         ship->flyingAI->enable();
      }
      break;


      //switch weapons
   case SDLK_v:
      // Keep scrolling through weapons as long as they're not purchased.
      clientCommand.currentWeapon = ship->getPrevWeaponID();
      break;

   case SDLK_z:
      // Keep scrolling through weapons as long as they're not purchased.
      clientCommand.currentWeapon = ship->getNextWeaponID();
      break;

   // Enable chat.
   case SDLK_j:
      chat->activate();
      break;

      // Minimap Display Size
   case SDLK_1:
      minimap->adjustDisplaySizeDirection = 1;
      break;
   case SDLK_2:
      minimap->adjustDisplaySizeDirection = -1;
      break;
   case SDLK_0:
      minimap->toggle();
      break;
      // Minimap Zoom
   case SDLK_3:
      minimap->disableAutoZoom();
      minimap->adjustZoomDirection = 1;
      break;
   case SDLK_4:
      minimap->disableAutoZoom();
      minimap->adjustZoomDirection = -1;
      break;
   case SDLK_5:
      minimap->toggleAutoZoom();
      break;


      // Audio and Video settings
   case SDLK_KP_ENTER:
      if (Mix_PausedMusic()) {
         Music::resumeMusic();
      } else {
         Music::pauseMusic();
      }
      break;


      // DEBUG KEYS
   case SDLK_F2:
      gameSettings->bloom = !gameSettings->bloom;
      break;
   case SDLK_F3:
      showBloomScreen = !showBloomScreen;
      break;
   case SDLK_F4:
      nextLevel();
      break;

   case SDLK_F6:
      testFunction();
      break;

   case SDLK_F7:
      drawGraphics = !drawGraphics;
      break;

   case SDLK_F8:
      debugPosition();
      break;
   case SDLK_F9:
      addAIPlayer();
      break;

   case SDLK_F10:
      ship->nShards += 10;
      ship->unbankedShards += 10;
      break;
      // If the user presses F11, give them all of the weapons.
   case SDLK_F11:
      // Make all of the weapons be purchased.
      for (int i = 0; i < ship->getNumWeapons(); i++) {
         ship->getWeapon(i)->purchased = true;
         if(ship->getWeapon(i)->curAmmo != -1) {
            ship->getWeapon(i)->curAmmo += 500;
         }
      }
      break;
      // Enables God Mode
   case SDLK_F12:
      godMode = !godMode;
      printf("Zoe mode: %d\n", godMode);
      break;
   }
}

/**
 * Handles the player letting go of a key
 */
void GameState::keyUp(int key) {
   //If game is not running, do not take input anymore
   if(!gameIsRunning || chat->chatActive){ return;}

   switch(key) {
      case SDLK_s:
         isS = false;
         if(!ship->flyingAI->isEnabled()) {
            if (isW) {
               clientCommand.forwardAcceleration = 1;
            } else {
               clientCommand.forwardAcceleration = 0;
            }
         }
         break;

      case SDLK_w:
         isW = false;
         if(!ship->flyingAI->isEnabled()) {
            if (isS) {
               clientCommand.forwardAcceleration = -1;
            } else {
               clientCommand.forwardAcceleration = 0;
            }
         }
         break;

      case SDLK_a:
         isA = false;
         if(!ship->flyingAI->isEnabled()) {
            if (isD) {
               clientCommand.yawSpeed = -1.0;
            } else {
               clientCommand.yawSpeed = 0;
            }
         }
         break;

      case SDLK_d:
         isD = false;
         if(!ship->flyingAI->isEnabled()) {
            if (isA) {
               clientCommand.yawSpeed = 1.0;
            } else {
               clientCommand.yawSpeed = 0;
            }
         }
         //if(!ship->flyingAI->isEnabled())
         //ship->setYawSpeed(0);
         break;

      case SDLK_q:
      case SDLK_e:
         if(!ship->flyingAI->isEnabled())
            clientCommand.rightAcceleration = 0;
         break;

      case SDLK_SPACE:
      case SDLK_LCTRL:
         if(!ship->flyingAI->isEnabled())
            clientCommand.upAcceleration = 0;
         break;

         /*
      case SDLK_LSHIFT:
         if(!ship->flyingAI->isEnabled())
            ship->setBoost(false);
         break;
   */

      case SDLK_b:
         clientCommand.brake = false;
         break;
         // Minimap Size
      case SDLK_1:
         minimap->adjustDisplaySizeDirection = 0;
         break;
      case SDLK_2:
         minimap->adjustDisplaySizeDirection = 0;
         break;
         // Minimap Zoom
      case SDLK_3:
         minimap->adjustZoomDirection = 0;
         break;
      case SDLK_4:
         minimap->adjustZoomDirection = 0;
         break;

         // Camera
      case SDLK_8:
      case SDLK_9:
         ship->setZoomSpeed(0);
         break;
   }
}

/**
 * Handles the player clicking the mouse
 */
void GameState::mouseDown(int button) {
   //If game is not running, do not take input anymore
   if(!gameIsRunning){ return;}

   switch(button) {
      // Left mouse down
   case 1:
      clientCommand.fire = true;
      break;

      // Right mouse down
   case 3:
      doYaw = !doYaw;
      clientCommand.rollSpeed = 0;
      clientCommand.yawSpeed = 0;
      break;

      // Scroll
   case 4:
      clientCommand.currentWeapon = ship->getPrevWeaponID();
      break;
   case 5:
      clientCommand.currentWeapon = ship->getNextWeaponID();
      break;
   }
}

/**
 * Handles the player letting go of a mouse click
 */
void GameState::mouseUp(int button) {
   //If game is not running, do not take input anymore
   if(!gameIsRunning){ return;}

   switch (button) {
      // Left mouse up
   case 1:
      clientCommand.fire = false;
      break;
      // Right mouse up
   case 3:
      doYaw = !doYaw;
      clientCommand.rollSpeed = 0;
      clientCommand.yawSpeed = 0;
      break;
   }
}

void GameState::mouseMove(int dx, int dy, int x, int y) {
   //If game is not running, do not take input anymore
   if(!gameIsRunning)
      return;

   shipControlX = p2wx(x);
   shipControlY = p2wy(y);

   mouseX = shipControlX;
   mouseY = shipControlY;
   
   clientCommand.mouseX = (float) mouseX;
   clientCommand.mouseY = (float) mouseY;

   shipControlX = shipControlX / p2wx(gameSettings->GW);
   shipControlY = shipControlY / p2wy(0);

   shipControlX = clamp(shipControlX * fabs(shipControlX * shipControlX), -1, 1);
   shipControlY = clamp(-shipControlY * fabs(shipControlY * shipControlY), -1, 1);

   if(!ship->flyingAI->isEnabled()) {
      if (doYaw) {
         clientCommand.yawSpeed = (float) -shipControlX;
      } else {
         clientCommand.rollSpeed = (float) shipControlX;
      }

      clientCommand.pitchSpeed = (float) shipControlY;
   }
}

double GameState::getWallxMin() const { return cube->xMin; }
double GameState::getWallxMax() const { return cube->xMax; }
double GameState::getWallyMin() const { return cube->yMin; }
double GameState::getWallyMax() const { return cube->yMax; }
double GameState::getWallzMin() const { return cube->zMin; }
double GameState::getWallzMax() const { return cube->zMax; }

double GameState::getMouseX() { return mouseX; }
double GameState::getMouseY() { return mouseY; }


Camera* GameState::getCurrentCamera() const {
   if (usingShipCamera) {
      return shipCamera;
   } else {
      return spectatorCamera;
   }
}

void GameState::debugPosition() {
   // First reset.
   reset();
   // Find the asteroid. Set its position.
   Asteroid3D* asteroid = NULL;
   std::vector<Object3D*>* objects = custodian.getListOfObjects();
   std::vector<Object3D*>::iterator iter = objects->begin();

   for (; iter != objects->end(); ++iter) {
      asteroid = dynamic_cast<Asteroid3D*>(*iter);
      if (asteroid != NULL)
         break;
   }

   ship->position->update(0, 0, 0);
   ship->forward->updateMagnitude(0, 1, 0);
   ship->up->updateMagnitude(0, 0, 1);
   ship->right->updateMagnitude(1, 0, 0);

   asteroid->position->update(0, 20, 0);
   asteroid->velocity->updateMagnitude(0, 0, 0);

   asteroid->update(0.01);
   asteroid->debug();

   // Make all of the weapons be purchased.
   for (int i = 0; i < ship->getNumWeapons(); i++) {
      ship->getWeapon(i)->purchased = true;
      if(ship->getWeapon(i)->curAmmo != -1) {
         ship->getWeapon(i)->curAmmo += 500;
      }
   }

}

void GameState::save() {
   std::ofstream ofs(SAVEFILENAME);
   ofs << "GameState: " << curLevel;
   ofs.close();
   std::cout << "Saved GameState" << std::endl;
}

void GameState::load() {
   std::ifstream ifs(SAVEFILENAME);

   std::string temp;
   ifs >> temp;
   ifs >> curLevel;

   ifs.close();
   std::cout << "Loaded GameState" << std::endl;
}

void GameState::testFunction() {
   NetTimer test;
   test.fromObject(levelTimer);

   std::ostringstream oss;
   boost::archive::text_oarchive oa(oss);
   int i = NET_OBJ_ASTEROID;
   oa << i << test;

   std::cout << "NetTimer=" << oss.str() << std::endl;

}

