/**
 * AsteroidShip.cpp
 * Sterling Hirsh
 * This is the ship that a player plays from or can see.
 */

#include "AsteroidShip.h"
#include <math.h>
#include <time.h>
#include "Utility/Quaternion.h"
#include "Utility/SoundEffect.h"
#include "Particles/EngineParticle.h"
#include "Particles/ElectricityImpactParticle.h"
#include "Text/GameMessage.h"
#include "Items/Spring.h"
#include "Network/gamestate.pb.h"
#include "Menus/StoreMenu.h"

#include <sstream>

// DEBUG
#include <fstream>

using namespace std;
extern ofstream debugoutput;


#ifndef M_PI
#define M_PI           3.14159265358979323846
#endif

#define BOOST_SCALE 2.0

#define SHOT_ANGLE_FACTOR ((M_PI/180) * VERT_FOV / 2)

#define KILLEDBY_DEFAULT 0

int TRACTOR_WEAPON_INDEX = 0;
int BLASTER_WEAPON_INDEX = 0;
int RAILGUN_WEAPON_INDEX = 0;
int ELECTRICITY_WEAPON_INDEX = 0;
int TIMEDBOMBER_WEAPON_INDEX = 0;
int REMOTEBOMBER_WEAPON_INDEX = 0;
int ENERGY_WEAPON_INDEX = 0;
int RAM_WEAPON_INDEX = 0;
int HOMINGMISSILE_WEAPON_INDEX = 0;

int NUMBER_OF_WEAPONS = 0;

const double rotationFactor = 2.6;
const float shipScale = 5;
const double rotationalAcceleration = 10; // rad/sec^2
const double spawnRate = .5;

/**
 * Constructor
 */
AsteroidShip::AsteroidShip(const GameState* _gameState) :
 Object3D(_gameState),
 bankTimer(gameState),
 name("UnnamedPlayer"),
 respawnTimer(gameState),
 aliveTimer(gameState)
{
   type = TYPE_ASTEROIDSHIP;

   cullRadius = 12;
   health = healthMax = 100;

   // Bounding box stuff.
   maxX = maxY = maxZ = 3;
   minX = minY = minZ = -3;
   radius = 3;
   updateBoundingBox();

   // Todo: comment these.
   spin = 90;
   flashiness = 0;
   
   tracker = 0;
   rando = 1;
   upOrDown = 1;

   zMove = 2;
   lineMove = zMove / 4;
   frontX = 0;
   frontY = 0;
   frontZ = 0;
   cornerX = .2;
   cornerY = .2;
   cornerZ = 1.3;
   middleXY = .15;
   middleZ = 1;
   backX = 0;
   backY = 0;
   backZ = 1.6;

   isBarrelRollingLeft = -1;
   isBarrelRollingRight = -1;

   hitX = 0;
   hitY = 0;
   hitZ = 0;

   drawShieldTime = 0.4; // Seconds
   justGotHit = 0;
   timeLeftToRespawn = -1;
   isFirstSpawn = true;

   //skew must be set to 0 til I figure out a better way to do things
   skew = 0;

   // TODO: Comment these.
   x2Change = middleXY / 2;
   y2Change = middleXY / 2;
   z2Change = (backZ - middleZ) / 2;
   xChange = 0;
   yChange = 0;
   zChange = 0;
   backChange = 0;

   shotOriginScale = 4;

   // The ship's score. This number is displayed to the screen.
   score = 0;
   kills = 0;
   deaths = 0;
   lives = PLAYER_LIVES;

   lastDamagerId = 0;
   killedBy = KILLEDBY_DEFAULT;
   lastDamagerWeapon = DAMAGER_INDEX_ASTEROID;

   // The number of shard collected. This number is displayed to the screen.
   bankedShards = 0;
   unbankedShards = 0;
   totalBankedShards = 0;
   bankPeriod = 10; // Default bank period is 10 seconds.
   bankTimer.reset();

   // The ship's max motion parameters.
   maxForwardAccel = 10;
   maxRightAccel = 20;
   maxUpAccel = 5;
   maxYawSpeed = maxPitchSpeed = maxRollSpeed = 3;

   // levels
   engineLevel = 1;
   regenHealthLevel = 0;
   bankLevel = 1;

   // Add weapons to the list!
   /* IF YOU CHANGE THE ORDER OF THIS LIST, CHANGE THE CONSTANTS IN Utility/Constants.h
   */
   int tmpNumberOfWeapons = 0;
   
   weapons.push_back(new TractorBeam(this, tmpNumberOfWeapons++));
   weapons.push_back(new Blaster(this, tmpNumberOfWeapons++));
   weapons.push_back(new HomingMissile(this, tmpNumberOfWeapons++));
   weapons.push_back(new Electricity(this, tmpNumberOfWeapons++));
   weapons.push_back(new Ram(this, tmpNumberOfWeapons++));
   //weapons.push_back(new LawnMower(this, tmpNumberOfWeapons++));
   weapons.push_back(new TimedBomber(this, tmpNumberOfWeapons++));
   //weapons.push_back(new RemoteBomber(this, tmpNumberOfWeapons++));
   weapons.push_back(new Energy(this, tmpNumberOfWeapons++));
   weapons.push_back(new RailGun(this, tmpNumberOfWeapons++));

   NUMBER_OF_WEAPONS = tmpNumberOfWeapons;

   // Create this ship's Radar
   radar = new Radar(this);

   soundHandle = NULL;

   cameraOffset = new Vector3D(0, 2, 5);
   currentView = VIEW_THIRDPERSON_SHIP;
   zoomFactor = 1.0;
   zoomSpeed = 0.0;

   // Ships should be drawn in the minimap
   shouldDrawInMinimap = true;

   collisionType = collisionSphere = new CollisionSphere(4, *position);

   reInitialize();

   /* These must be created last b/c they need the ship / weapons to be
    * initialized first.
    */
   shooter = new ShootingAI(this);
   flyingAI = new FlyingAI(this);
   
   // The two colors of the ship
   color1 = (float) randdouble();
   color2 = (float) randdouble();
}

/**
 * Destructor
 */
AsteroidShip::~AsteroidShip() {
   for (unsigned int i = 0; i < weapons.size(); ++i) {
      delete (weapons[i]);
   }
   delete shooter;
   delete flyingAI;
   delete radar;
   delete cameraOffset;
}

/**
 * Reset the ship as if the game just started.
 */
void AsteroidShip::reset() {
   reInitialize();

   if (gameState->gsm != ClientMode) {
      lives = PLAYER_LIVES;
      kills = deaths = score = 0;
   }
}

/**
 * Reset the ship as if it just spawned.
 */
void AsteroidShip::reInitialize() {
   // DEBUG
   debugoutput << "reinitializing!" << endl;
   shakeAmount = 0;
   
   /* We store acceleration as scalars to multiply forward, right, and up by each tick. */
   curForwardAccel = curRightAccel = curUpAccel = 0;
   yawSpeed = rollSpeed = pitchSpeed = 0;
   targetYawSpeed = targetRollSpeed = targetPitchSpeed = 0;
   maxSpeed = 5; // Units/s, probably will be changed with an upgrade.
   
   /* Currently not braking or acceleration. */
   isBraking = false;
   brakeFactor = 2;

   shotPhi = shotTheta = 0;

   // The ship's currently selected weapon.
   currentWeapon = BLASTER_WEAPON_INDEX; // Blaster

   isBarrelRollingLeft = -1;
   isBarrelRollingRight = -1;

   accelerationStartTime = gameState->getGameTime();
   particlesEmitted = 0;
   
   deathAcknowledged = true;

   interpolateOrientation = false;
   orientationInterpolationAmount = 0;

   if (gameState->gsm != ClientMode) {
      // Is the ship firing? Not when it's instantiated.
      isFiring = false;

      health = healthMax;
   
      // This does its own gsm check.
      randomizePosition();

      velocity->updateMagnitude(0, 0, 0);

      respawnTimer.reset();
      aliveTimer.countUp();
      
      // Reset powerups.
      healthMax = 100;
      engineLevel = 1;
      regenHealthLevel = 0;
      bankLevel = 1;
      killedBy = KILLEDBY_DEFAULT;
   }
}

/**
 * Set the currently selected weapon to the weapon type specified.
 * The currentWeapon can also be set from nextWeapon() and prevWeapon().
 */
void AsteroidShip::selectWeapon(int weaponType) {
   if (currentWeapon != weaponType) {
      currentWeapon = weaponType;
      weapons[currentWeapon]->activationTimer.setCountDown(0.5);
   }
}

/**
 * Retrieve the ship's health.
 */
double AsteroidShip::getHealth() {
   return health;
}

/**
 * Retrieve a pointer to the ship's shot direction vector
 */
Vector3D* AsteroidShip::getShotDirection() {
   return &shotDirection;
}

/**
 * Retrieve the ship's score.
 */
int AsteroidShip::getScore() {
   return score;
}

/**
 * Retrieve the number of shards collected.
 */
int AsteroidShip::getShards() {
   return bankedShards;
}

void AsteroidShip::setYawSpeed(double yawAmountIn) {
   targetYawSpeed = rotationFactor * yawAmountIn;
}

void AsteroidShip::setPitchSpeed(double pitchAmountIn) {
   targetPitchSpeed = rotationFactor * pitchAmountIn;
}

void AsteroidShip::setRollSpeed(double rollAmountIn) {
   targetRollSpeed = rotationFactor * rollAmountIn;
}

void AsteroidShip::updatePlayerAcceleration() {
   if (gameState->gsm != ClientMode) {
      Vector3D* newAcceleration = new Vector3D(
            forward->scalarMultiply(curForwardAccel).add(
               right->scalarMultiply(curRightAccel).add(
                  up->scalarMultiply(curUpAccel))));
      Vector3D normalizedAccel = newAcceleration->getNormalized();
      double topAccelSpeed = maxSpeed + engineLevel * 2;
      if (isFiring && currentWeapon == RAM_WEAPON_INDEX  && weapons[RAM_WEAPON_INDEX]->isReady()) {
         topAccelSpeed *= 4;
      }
      if (normalizedAccel.dot(*velocity) < topAccelSpeed) {
         addAcceleration(newAcceleration);
      }
   }
}

void AsteroidShip::setBrake(bool doBrake) {
   isBraking = doBrake;
}

/**
 * Set the engine's acceleration.
 */
void AsteroidShip::accelerateForward(int dir) {
   if (curForwardAccel == 0 && dir != 0) {
      accelerationStartTime = gameState->getGameTime();
      particlesEmitted = 0;
   }

   curForwardAccel = dir * maxForwardAccel;
   updatePlayerAcceleration();
}

void AsteroidShip::accelerateUp(int dir) {
   if (curUpAccel == 0 && dir != 0) {
      accelerationStartTime = gameState->getGameTime();
      particlesEmitted = 0;
   }

   curUpAccel = dir * maxUpAccel;
   updatePlayerAcceleration();
}

void AsteroidShip::accelerateRight(int dir) {
   if (curRightAccel == 0 && dir != 0) {
      accelerationStartTime = gameState->getGameTime();
      particlesEmitted = 0;
   }

   curRightAccel = dir * maxRightAccel;
   updatePlayerAcceleration();
}

void AsteroidShip::addNewParticle(Point3D& emitter, Vector3D& baseDirection,
      Vector3D& offsetDirectionX, Vector3D& offsetDirectionY, double color) {
   static Vector3D particleVariation;
   static Point3D curPoint;
   static Vector3D initialOffset;
   static Vector3D randomOffset;
   const float randomAmount = 0.5f;
   curPoint = emitter;

   // Translate the point in 2D
   randomOffset.add(offsetDirectionX.scalarMultiply(randomAmount * (randdouble() - 0.5)));
   randomOffset.add(offsetDirectionY.scalarMultiply(randomAmount * (randdouble() - 0.5)));
   randomOffset.add(baseDirection.scalarMultiply(randomAmount * (randdouble() -0.5)));
   //randomOffset.scalarMultiplyUpdate(0.01);

   particleVariation.updateMagnitude(baseDirection.scalarMultiply(randdouble() * 6));
   particleVariation.addUpdate(offsetDirectionX.scalarMultiply(randdouble() * 8 - 4));
   particleVariation.addUpdate(offsetDirectionY.scalarMultiply(randdouble() * 8 - 4));
   particleVariation.scalarMultiplyUpdate(0.2);
   //curPoint = position->add(randomPoint);
   initialOffset.movePoint(curPoint);
   randomOffset.movePoint(curPoint);
   EngineParticle::Add(new Point3D(curPoint),
         new Vector3D(baseDirection.scalarMultiply(10.0).add(particleVariation)), color, gameState);
}

void AsteroidShip::addNewLowHealthParticle(Point3D& emitter, Vector3D& baseDirection,
      Vector3D& offsetDirectionX, Vector3D& offsetDirectionY, double color) {
   static Vector3D particleVariation;
   static Point3D curPoint;
   static Vector3D initialOffset;
   static Vector3D randomOffset;
   const float randomAmount = 0.5f;
   curPoint = emitter;

   // Translate the point in 2D
   randomOffset.add(offsetDirectionX.scalarMultiply(randomAmount * (randdouble() - 0.5)));
   randomOffset.add(offsetDirectionY.scalarMultiply(randomAmount * (randdouble() - 0.5)));
   randomOffset.add(baseDirection.scalarMultiply(randomAmount * (randdouble() -0.5)));
   //randomOffset.scalarMultiplyUpdate(0.01);

   particleVariation.updateMagnitude(baseDirection.scalarMultiply(randdouble() * 30));
   particleVariation.addUpdate(offsetDirectionX.scalarMultiply(randdouble() * 8 - 4));
   particleVariation.addUpdate(offsetDirectionY.scalarMultiply(randdouble() * 8 - 4));
   particleVariation.scalarMultiplyUpdate(0.2);
   //curPoint = position->add(randomPoint);
   initialOffset.movePoint(curPoint);
   randomOffset.movePoint(curPoint);
   EngineParticle::AddLowHealth(new Point3D(curPoint),
         new Vector3D(baseDirection.scalarMultiply(10.0).add(particleVariation)), color, gameState);
}

void AsteroidShip::createEngineParticles(double timeDiff) {
   //add particles in the opposite direction of the acceration

   int maxParticlesPerFrame = 10;
   int newParticlesPerSecond = 50;
   Vector3D baseParticleAcceleration;
   Point3D emitter;

   double accelerationTime = gameState->getGameTime() - accelerationStartTime;
   double colorVariation = 0.2 * randdouble();
   int particlesThisFrame = 0;

   while ((double) particlesEmitted / accelerationTime < newParticlesPerSecond &&
         particlesThisFrame < maxParticlesPerFrame) {
      //printf("Totally getting here %f\n", particlesThisFrame);
      // First do up Acceleration.
      if (curUpAccel != 0) {
         baseParticleAcceleration = up->scalarMultiply(-curUpAccel * 0.2);
         emitter = *position;
         forward->movePoint(emitter, -0.5);
         addNewParticle(emitter, baseParticleAcceleration, *forward, *right, color1 + colorVariation);
      }

      // Next do right Acceleration.
      if (curRightAccel != 0) {
         if(curRightAccel > 10) {
            baseParticleAcceleration = right->scalarMultiply(-10 * 0.2);
         } else if (curRightAccel < -10) {
            baseParticleAcceleration = right->scalarMultiply(10 * 0.2);
         } else {
            baseParticleAcceleration = right->scalarMultiply(-curRightAccel * 0.2);
         }
         emitter = *position;
         forward->movePoint(emitter, -0.7);
         addNewParticle(emitter, baseParticleAcceleration, *forward, *up, color2 + colorVariation);
      }

      // Next do forward Acceleration.
      if (curForwardAccel != 0) {
         // We want to do two streams.
         baseParticleAcceleration = forward->scalarMultiply(-curForwardAccel * 0.05);
         Point3D initialPoint(*position);
         forward->movePoint(initialPoint,  -(curForwardAccel * 0.07));

         // First do the right side.
         right->movePoint(initialPoint, 1);
         addNewParticle(initialPoint, baseParticleAcceleration, *right, *up, color1 - colorVariation);

         // Next do the left side.
         right->movePoint(initialPoint, -2);
         addNewParticle(initialPoint, baseParticleAcceleration, *right, *up, color2 - colorVariation);
      }

      ++particlesEmitted;
      ++particlesThisFrame;
   }
}

void AsteroidShip::createLowHealthParticles(double timeDiff){
   //add particles in the opposite direction of the acceration
   int maxParticlesPerFrame = 8;
   Vector3D baseParticleAcceleration;
   Point3D emitter;

   double colorVariation = 0.2 * randdouble();
   int particlesThisFrame = 0;

   while (particlesThisFrame < maxParticlesPerFrame && particlesThisFrame < ((50 - health) /5)) {
      // First do up Acceleration.
      baseParticleAcceleration = up->scalarMultiply(.5 * (.5 - randdouble())) + right->scalarMultiply(.5 * (.5 - randdouble()));
      
      emitter = *position;
      
      forward->movePoint(emitter, -0.5);
      
      addNewLowHealthParticle(emitter, baseParticleAcceleration, *forward, *right, color1 + colorVariation);
      ++particlesThisFrame;
   }
}

void AsteroidShip::addKillMessage() {
   Object3D* killer = (*custodian)[lastDamagerId];
   if (killer != NULL) {
      ostringstream killMessage;
      if (killer->type == TYPE_ASTEROIDSHIP) {
         AsteroidShip* killerShip = static_cast<AsteroidShip*>(killer);
         if (gameState->gsm != ClientMode) {
            killerShip->kills++;
         }
         killMessage << killerShip->name << " killed " << name << " with a " 
          << weapons[lastDamagerWeapon]->getName() << ".";
      } else {
         killMessage << name << " flew into an asteroid.";
      }

      // Add the kill message to the list.
      GameMessage::Add(killMessage.str(), 15, 5, gameState, true);
   } else {
      debugoutput << "Null killer!" << endl;
   }
}

void AsteroidShip::addRespawnMessage() {
   ostringstream gameMsg;
   gameMsg << "Respawning in " << (int)(timeLeftToRespawn);
   GameMessage::Add(gameMsg.str(), 30, 0, gameState);

}

void AsteroidShip::update(double timeDiff) {
   if (health <= 0) {
      doDeadStuff(timeDiff);

      if (health <= 0) {
         // Must not have respawned here.
         return;
      }
   } else if (lives <= 0) {
      shouldDrawInMinimap = false;
      zeroControls();
      
      if (this == gameState->ship) {
         gameState->usingShipCamera = false;
      }
   } else if (!shouldDrawInMinimap) {
      // If health > 0.
      shouldDrawInMinimap = true;
   }


   // We don't make it past here if health is <= 0.
   
   if (this == gameState->ship && health > 0 && lives > 0) {
      gameState->usingShipCamera = true;
   }


   if (lives > 0) {
      if (timeLeftToRespawn > 0 && (gameState->gsm != MenuMode)) {
         timeLeftToRespawn -= timeDiff;
         drawSpawn = true;
         return;
      } else {
         drawSpawn = false;
      }

      if (gameState->gsm != ClientMode) {
         doRegenHealth(timeDiff);
         
         if (shooter->isEnabled()) {
            shooter->think(timeDiff);
         }

         if (flyingAI->isEnabled()) {
            flyingAI->think(timeDiff);
         } else {
            updatePlayerAcceleration();
         }

         doBraking(timeDiff);
      }
   }

   Object3D::update(timeDiff);         
   
   if (lives > 0) {
      limitVelocity(timeDiff);
      applyRotationalAcceleration(timeDiff); // Only if not barrel rolling
      doInterpolateOrientation(timeDiff);

      if (!shooter->isEnabled()) {
         updateShotDirectionVector();
      }

      if (gameState->gsm == SingleMode) {
         bankShards();
      }
   }

   updateWeapons(timeDiff);

   if (lives > 0) {
      updateSound();

      shotOrigin = *position;
      // I don't think anything does fireBackwards.
      shotOriginScale = 4;
      forward->movePoint(shotOrigin, shotOriginScale);

      // Actually fire the weapons.
      keepFiring(); // This calls fire() on the right weapon.

      reduceShake(timeDiff);
      updateSpaceBoner(timeDiff);

      drawHit = (gameState->getGameTime() - justGotHit) < drawShieldTime;

      handleBarrelRoll(timeDiff);
   
      handleRamShotEffect(timeDiff);

      if (timeDiff > 0) {
         createEngineParticles(timeDiff);
         if (health < 50)
            createLowHealthParticles(timeDiff);
      }

      updateZoomLevel(timeDiff);
   }
}

/**
 * This translates an xOffset and yOffset in world coords to
 * a phi and a theta from the ship.
 */
void AsteroidShip::updateShotDirection(double xOffset, double yOffset) {
   aimX = xOffset;
   aimY = yOffset;
   shotPhi = yOffset * SHOT_ANGLE_FACTOR;
   shotTheta = -xOffset * SHOT_ANGLE_FACTOR;
   updateShotDirectionVector();
}

void AsteroidShip::fire(bool startFiring) {
   isFiring = startFiring;
}

void AsteroidShip::keepFiring() {
   if (!isFiring) return;
   weapons[currentWeapon]->fire();
}

void AsteroidShip::draw_frontpanels() {
   //fboBegin();
   GLenum buffers[] = {ALBEDO_BUFFER, NORMAL_BUFFER, GLOW_BUFFER, NOLIGHT_BUFFER};
   glDrawBuffers(4, buffers);

   glUseProgram(shipYShader);

   float r, g, b;
   getBrightColor(color1, r, g, b);
   glColor3d(r, g, b);

   glUseProgram(shipYShader);

   glBegin(GL_TRIANGLES);
   // Right panel, top half.
   Point3D p1 = Point3D(frontX, frontY, frontZ);
   Point3D p2 = Point3D(cornerX, cornerY, cornerZ);
   Point3D p3 = Point3D(middleXY, skew, middleZ);
   Vector3D s1 = p1 - p2;
   Vector3D s2 = p3 - p2;
   Vector3D normal = s2.cross(s1);
   normal.addNormal();
   glVertex3d(frontX, frontY, frontZ);
   glVertex3d(cornerX, cornerY, cornerZ);
   glVertex3d(middleXY, skew, middleZ);

   // Left panel, top half.
   p1 = Point3D(frontX, frontY, frontZ);
   p2 = Point3D(-middleXY, skew, middleZ);
   p3 = Point3D(-cornerX, cornerY, cornerZ);
   s1 = p1 - p2;
   s2 = p3 - p2;
   normal = s1.cross(s2);
   normal.addNormal();
   glVertex3d(frontX, frontY, frontZ);
   glVertex3d(-middleXY, skew, middleZ);
   glVertex3d(-cornerX, cornerY, cornerZ);

   // Left panel, bottom half.
   p1 = Point3D(frontX, frontY, frontZ);
   p2 = Point3D(-cornerX, -cornerY, cornerZ);
   p3 = Point3D(-middleXY, skew, middleZ);
   s1 = p1 - p2;
   s2 = p3 - p2;
   normal = s2.cross(s1);
   normal.addNormal();
   glVertex3d(frontX, frontY, frontZ);
   glVertex3d(-cornerX, -cornerY, cornerZ);
   glVertex3d(-middleXY, skew, middleZ);

   // Right panel, bottom half.
   p1 = Point3D(frontX, frontY, frontZ);
   p2 = Point3D(middleXY, skew, middleZ);
   p3 = Point3D(cornerX, -cornerY, cornerZ);
   s1 = p1 - p2;
   s2 = p3 - p2;
   normal = s1.cross(s2);
   normal.addNormal();
   glVertex3d(frontX, frontY, frontZ);
   glVertex3d(middleXY, skew, middleZ);
   glVertex3d(cornerX, -cornerY, cornerZ);
   glEnd();
   glUseProgram(0);

   glUseProgram(shipXShader);
   glBegin(GL_TRIANGLES);

   // Top panel, right half.
   p1 = Point3D(frontX, frontY, frontZ);
   p2 = Point3D(skew, middleXY, middleZ);
   p3 = Point3D(cornerX, cornerY, cornerZ);
   s1 = p1 - p2;
   s2 = p3 - p2;
   normal = s2.cross(s1);
   normal.addNormal();
   glVertex3d(frontX, frontY, frontZ);
   glVertex3d(skew, middleXY, middleZ);
   glVertex3d(cornerX, cornerY, cornerZ);

   // Top panel, left half.
   p1 = Point3D(frontX, frontY, frontZ);
   p2 = Point3D(-cornerX, cornerY, cornerZ);
   p3 = Point3D(skew, middleXY, middleZ);
   s1 = p1 - p2;
   s2 = p3 - p2;
   normal = s2.cross(s1);
   normal.addNormal();
   glVertex3d(frontX, frontY, frontZ);
   glVertex3d(-cornerX, cornerY, cornerZ);
   glVertex3d(skew, middleXY, middleZ);

   // Bottom panel, left half.
   p1 = Point3D(frontX, frontY, frontZ);
   p2 = Point3D(skew, -middleXY, middleZ);
   p3 = Point3D(-cornerX, -cornerY, cornerZ);
   s1 = p1 - p2;
   s2 = p3 - p2;
   normal = s2.cross(s1);
   normal.addNormal();
   glVertex3d(frontX, frontY, frontZ);
   glVertex3d(skew, -middleXY, middleZ);
   glVertex3d(-cornerX, -cornerY, cornerZ);

   // Bottom panel, right half.
   p1 = Point3D(frontX, frontY, frontZ);
   p2 = Point3D(cornerX, -cornerY, cornerZ);
   p3 = Point3D(skew, -middleXY, middleZ);
   s1 = p1 - p2;
   s2 = p3 - p2;
   normal = s2.cross(s1);
   normal.addNormal();
   glVertex3d(frontX, frontY, frontZ);
   glVertex3d(cornerX, -cornerY, cornerZ);
   glVertex3d(skew, -middleXY, middleZ);
   glEnd();
   glUseProgram(0);
   //fboEnd();
}

void AsteroidShip::draw_backpanels() {
   float r, g, b;
   getBrightColor(color2, r, g, b);
   glColor3d(r, g, b);

   //fboBegin();
   GLenum buffers[] = {ALBEDO_BUFFER, NORMAL_BUFFER, GLOW_BUFFER, NOLIGHT_BUFFER};
   glDrawBuffers(4, buffers);

   glUseProgram(backShader);
   glBegin(GL_TRIANGLES);

   // Top right corner panel.
   Point3D p1 = Point3D(middleXY, skew, middleZ);
   Point3D p2 = Point3D(cornerX, cornerY, cornerZ);
   Point3D p3 = Point3D(skew, middleXY, middleZ);
   Vector3D s1 = p1 - p2;
   Vector3D s2 = p3 - p2;
   Vector3D normal = s2.cross(s1);
   normal.addNormal();
   glVertex3d(middleXY, skew, middleZ);
   glVertex3d(cornerX, cornerY, cornerZ);
   glVertex3d(skew, middleXY, middleZ);

   // Bottom right corner panel.
   p1 = Point3D(middleXY, skew, middleZ);
   p2 = Point3D(skew, -middleXY, middleZ);
   p3 = Point3D(cornerX, -cornerY, cornerZ);
   s1 = p1 - p2;
   s2 = p3 - p2;
   normal = s2.cross(s1);
   normal.addNormal();
   glVertex3d(middleXY, skew, middleZ);
   glVertex3d(skew, -middleXY, middleZ);
   glVertex3d(cornerX, -cornerY, cornerZ);

   // Bottom left corner panel.
   p1 = Point3D(-middleXY, skew, middleZ);
   p2 = Point3D(cornerX, -cornerY, cornerZ);
   p3 = Point3D(skew, -middleXY, middleZ);
   s1 = p1 - p2;
   s2 = p3 - p2;
   normal = s2.cross(s1);
   normal.addNormal();
   glVertex3d(-middleXY, skew, middleZ);
   glVertex3d(-cornerX, -cornerY, cornerZ);
   glVertex3d(skew, -middleXY, middleZ);

   // Top left corner panel.
   p1 = Point3D(-middleXY, skew, middleZ);
   p2 = Point3D(skew, middleXY, middleZ);
   p3 = Point3D(-cornerX, cornerY, cornerZ);
   s1 = p1 - p2;
   s2 = p3 - p2;
   normal = s2.cross(s1);
   normal.addNormal();
   glVertex3d(-middleXY, skew, middleZ);
   glVertex3d(skew, middleXY, middleZ);
   glVertex3d(-cornerX, cornerY, cornerZ);

   glEnd();
   glUseProgram(0);
   //fboEnd();
}

void AsteroidShip::draw_spaceboner() {
   //fboBegin();
   if (gameSettings->drawDeferred) {
      GLenum buffers[] = {NORMAL_BUFFER, ALBEDO_BUFFER, GLOW_BUFFER, NOLIGHT_BUFFER};
      glDrawBuffers(4, buffers);

      glUseProgram(bonerShader);
   }
   glBegin(GL_TRIANGLES);
   if (curForwardAccel == 10.0) {
      glColor4d(1, .4, 0, 1);

      glVertex3d(middleXY, skew, middleZ);
      glVertex3d(skew, middleXY, middleZ);
      glVertex3d(backX, backY, middleZ + backChange);

      glVertex3d(middleXY, skew, middleZ);
      glVertex3d(backX, backY, middleZ + backChange);
      glVertex3d(skew, -middleXY, middleZ);

      glVertex3d(-middleXY, skew, middleZ);
      glVertex3d(skew, -middleXY, middleZ);
      glVertex3d(backX, backY, middleZ + backChange);

      glVertex3d(-middleXY, skew, middleZ);
      glVertex3d(skew, middleXY, middleZ);
      glVertex3d(backX, backY, middleZ + backChange);

   } else if (backChange == 0) {
      glColor4d(0, 0, 0, 1);

      glVertex3d(-middleXY, skew, middleZ);
      glVertex3d(middleXY, skew, middleZ);
      glVertex3d(skew, middleXY, middleZ);

      glVertex3d(-middleXY, skew, middleZ);
      glVertex3d(middleXY, skew, middleZ);
      glVertex3d(skew, -middleXY, middleZ);
   } else {
      glColor4d(1, .4, 0, 1);

      glVertex3d(middleXY, skew, middleZ);
      glVertex3d(skew, middleXY, middleZ);
      glVertex3d(backX, backY, middleZ + backChange);

      glVertex3d(middleXY, skew, middleZ);
      glVertex3d(backX, backY, middleZ + backChange);
      glVertex3d(skew, -middleXY, middleZ);

      glVertex3d(-middleXY, skew, middleZ);
      glVertex3d(skew, -middleXY, middleZ);
      glVertex3d(backX, backY, middleZ + backChange);

      glVertex3d(-middleXY, skew, middleZ);
      glVertex3d(backX, backY, middleZ + backChange);
      glVertex3d(skew, middleXY, middleZ);
   }
   glEnd();
   if (gameSettings->drawDeferred) {
      glUseProgram(0);
   }
   //fboEnd();
}

void AsteroidShip::draw_bonerlines() {
   float r, g, b;
   getBrightColor(color2, r, g, b);

   //fboBegin();
   GLenum buffers[] = {GLOW_BUFFER, NOLIGHT_BUFFER};
   glDrawBuffers(2, buffers);

   if (curForwardAccel == 10.0) {
      glLineWidth(1.0);
      if (backChange == (backZ - middleZ)) {
         glBegin(GL_LINE_LOOP);
         glColor3d(1, 0, 0);
         glVertex3d(middleXY - xChange, skew, middleZ + zChange);
         glVertex3d(skew, middleXY - yChange, middleZ + zChange);
         glVertex3d(-middleXY + xChange, skew, middleZ + zChange);
         glVertex3d(skew, -middleXY + yChange, middleZ + zChange);
         glVertex3d(middleXY - xChange, skew, middleZ + zChange);
         glEnd();

         glBegin(GL_LINE_LOOP);
         glVertex3d(middleXY - x2Change, skew, middleZ + z2Change);
         glVertex3d(skew, middleXY - y2Change, middleZ + z2Change);
         glVertex3d(-middleXY + x2Change, skew, middleZ + z2Change);
         glVertex3d(skew, -middleXY + y2Change, middleZ + z2Change);
         glVertex3d(middleXY - x2Change, skew, middleZ + z2Change);
         glEnd();

      }
      glBegin(GL_LINE_LOOP);
      glColor3d(1, 0, 0);

      glVertex3d(middleXY, skew, middleZ);
      glVertex3d(cornerX, cornerY, cornerZ);
      glVertex3d(skew, middleXY, middleZ);

      glVertex3d(middleXY, skew, middleZ);
      glVertex3d(backX, backY, middleZ + backChange);
      glVertex3d(skew, middleXY, middleZ);

      glVertex3d(middleXY, skew, middleZ);
      glVertex3d(cornerX, -cornerY, cornerZ);
      glVertex3d(skew, -middleXY, middleZ);

      glVertex3d(middleXY, skew, middleZ);
      glVertex3d(backX, backY, middleZ + backChange);
      glVertex3d(skew, -middleXY, middleZ);

      glVertex3d(-middleXY, skew, middleZ);
      glVertex3d(-cornerX, -cornerY, cornerZ);
      glVertex3d(skew, -middleXY, middleZ);

      glVertex3d(-middleXY, skew, middleZ);
      glVertex3d(backX, backY, middleZ + backChange);
      glVertex3d(skew, -middleXY, middleZ);

      glVertex3d(-middleXY, skew, middleZ);
      glVertex3d(-cornerX, cornerY, cornerZ);
      glVertex3d(skew, middleXY, middleZ);

      glVertex3d(-middleXY, skew, middleZ);
      glVertex3d(backX, backY, middleZ + backChange);
      glVertex3d(skew, middleXY, middleZ);

      glEnd();
   } else if (backChange == 0) {
      glBegin(GL_LINE_LOOP);
      glColor3d(r, g, b);
      glVertex3d(middleXY, skew, middleZ);
      glVertex3d(skew, middleXY, middleZ);
      glVertex3d(-middleXY, skew, middleZ);
      glVertex3d(skew, -middleXY, middleZ);
      glVertex3d(middleXY, skew, middleZ);
      glEnd();
   } else {
      glLineWidth(1.0);
      glBegin(GL_LINE_LOOP);
      glColor3d(r, g, b);
      glVertex3d(middleXY, skew, middleZ);
      glVertex3d(backX, backY, middleZ + backChange);
      glVertex3d(skew, middleXY, middleZ);

      glVertex3d(middleXY, skew, middleZ);
      glVertex3d(skew, -middleXY, middleZ);
      glVertex3d(backX, backY, middleZ + backChange);


      glVertex3d(-middleXY, skew, middleZ);
      glVertex3d(skew, -middleXY, middleZ);
      glVertex3d(backX, backY, middleZ + backChange);


      glVertex3d(-middleXY, skew, middleZ);
      glVertex3d(backX, backY, middleZ + backChange);
      glVertex3d(skew, middleXY, middleZ);
      glEnd();

      glBegin(GL_LINE_LOOP);
      glColor3d(r, g, b);
      glVertex3d(middleXY, skew, middleZ);
      glVertex3d(skew, middleXY, middleZ);
      glVertex3d(-middleXY, skew, middleZ);
      glVertex3d(skew, -middleXY, middleZ);
      glVertex3d(middleXY, skew, middleZ);
      glEnd();
   }
   glLineWidth(1.0);
   //fboEnd();
}

void AsteroidShip::draw_frontlines() {
   float r, g, b;
   getBrightColor(color1, r, g, b);

   //fboBegin();
   GLenum buffers[] = {ALBEDO_BUFFER, GLOW_BUFFER};
   glDrawBuffers(2, buffers);

   glBegin(GL_LINES);
   glColor3d(r, g, b);
   glVertex3d(frontX, frontY, frontZ);
   glVertex3d(cornerX, cornerY, cornerZ);

   glVertex3d(frontX, frontY, frontZ);
   glVertex3d(-cornerX, cornerY, cornerZ);

   glVertex3d(frontX, frontY, frontZ);
   glVertex3d(-cornerX, -cornerY, cornerZ);

   glVertex3d(frontX, frontY, frontZ);
   glVertex3d(cornerX, -cornerY, cornerZ);
   glEnd();

   //fboEnd();
}

void AsteroidShip::draw_backlines() {
   //fboBegin();
   GLenum buffers[] = {ALBEDO_BUFFER, GLOW_BUFFER};
   glDrawBuffers(2, buffers);

   glBegin(GL_LINE_LOOP);
   glVertex3d(middleXY, skew, middleZ);
   glVertex3d(cornerX, cornerY, cornerZ);
   glVertex3d(skew, middleXY, middleZ);
   glVertex3d(-cornerX, cornerY, cornerZ);
   glVertex3d(-middleXY, skew, middleZ);
   glVertex3d(-cornerX, -cornerY, cornerZ);
   glVertex3d(skew, -middleXY, middleZ);
   glVertex3d(cornerX, -cornerY, cornerZ);
   glVertex3d(middleXY, skew, middleZ);
   glEnd();

   //fboEnd();
}

void AsteroidShip::draw_spawn() {
   if (!isFirstSpawn && drawSpawn) {
      glPushMatrix();
      if (timeLeftToRespawn > 2 * spawnRate) {
         glScaled(.05, .05, (3 * spawnRate - timeLeftToRespawn) / spawnRate);
      } else if (timeLeftToRespawn > spawnRate) {
         glScaled((2 * spawnRate - timeLeftToRespawn) / spawnRate, .05, 1);
      } else if (timeLeftToRespawn > 0) {
         glScaled(1, (spawnRate - timeLeftToRespawn) / spawnRate, 1);
      }
      draw_ship();
      glPopMatrix();
   } else {
      glPushMatrix();
      if (aliveTimer.getTimeRunning() < (spawnRate)) {
         glScaled(.05, .05, (aliveTimer.getTimeRunning()) / spawnRate);
      } else if (aliveTimer.getTimeRunning() < (2 * spawnRate)) {
         glScaled((aliveTimer.getTimeRunning() - spawnRate) / spawnRate, .05, 1);
      } else if (aliveTimer.getTimeRunning() < (3 * spawnRate)) {
         glScaled(1, (aliveTimer.getTimeRunning() - 2 * spawnRate) / spawnRate, 1);
      } else if (aliveTimer.getTimeRunning() > (6 * spawnRate)) {
         isFirstSpawn = false;
      }
      draw_ship();
      glPopMatrix();
   }
   if (drawSpawn) {
      glPushMatrix();
      glTranslated(0, 0, -4);
      glScaled(7.5, 2.5, 4);
      //fboBegin();
      glDrawBuffer(ALBEDO_BUFFER);
      draw_hitEffect();
      //fboEnd();
      glPopMatrix();
   }
}

void AsteroidShip::draw_ship() {
   glPolygonOffset(1.0f, 1.0f);
   glEnable(GL_POLYGON_OFFSET_FILL);
   glPolygonMode(GL_FRONT_AND_BACK, GL_FILL);

   glDisable(GL_CULL_FACE);
   glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
   glDisable(GL_LIGHTING);
   glEnable(GL_COLOR_MATERIAL);

   // Make the ship start off flat and expand it to its normal size.

   glTranslated(0, 0, -4);

   glScaled(1.5, .5, .8);
   glScaled(shipScale, shipScale, shipScale);

   setMaterial(BlackSolid);

   glEnable(GL_NORMALIZE);

   draw_frontpanels();

   draw_backpanels();

   draw_spaceboner();

   /* Outline of Ship */
   glEnable(GL_POLYGON_OFFSET_LINE);
   glPolygonOffset(-1.0f, -1.0f);
   glPolygonMode(GL_FRONT_AND_BACK, GL_LINE);

   glLineWidth(1.5);
   //glDisable(GL_LIGHTING);

   draw_bonerlines();

   draw_frontlines();

   draw_backlines();

   glEnable(GL_LIGHTING);
   glDisable(GL_COLOR_MATERIAL);
   glEnable(GL_CULL_FACE);
   glDisable(GL_POLYGON_OFFSET_LINE);
   glPolygonMode(GL_FRONT_AND_BACK, GL_FILL);
}

void draw_vectors(){
   setMaterial(WhiteSolid);
   glBegin(GL_LINES);
   glVertex3d(0,0,0);
   glVertex3d(0,.5,0);

   glVertex3d(0,0,0);
   glVertex3d(0,0,-1);

   glVertex3d(.5,0,0);
   glVertex3d(-.5,0,0);

   glEnd();

   glLineWidth(1.0);
}

/**
 * Draw an adorable little ship in the minimap.
 */
void AsteroidShip::drawInMinimap() {
   glPushMatrix();
   position->glTranslate();
   //shotOrigin->glTranslate();
   // Counteract the rotation done in GameState::drawInMinimap();
   glRotate();
   //glScalef(shipScale, shipScale, shipScale);
   //glTranslatef(0, 0, -3);
   glColor4d(0, 0, 0, 0.2);
   draw_ship();
   glPopMatrix();
}

void AsteroidShip::draw_hitEffect() {
   hitX += (double) (rand() % 10);
   hitY += (double) (rand() % 20);
   hitZ += (double) (rand() % 30);

   double sx, sy, sz;
   sx = 5 / (1.5 * 5);
   sy = 5 / (.5 * 5);
   sz = 5 / (.8 * 5);

   if (hitX > 100) {
      hitX = 0;
   }
   if (hitY > 100) {
      hitY = 0;
   }
   if (hitZ > 100) {
      hitZ = 0;
   }
   glPolygonMode(GL_FRONT_AND_BACK, GL_FILL);
   glUseProgram(hitShader);
   glPushMatrix();
   glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
   glDisable(GL_LIGHTING);
   //printf("I got into draw hit effect\n");
   glScaled(sx, sy, sz);

   glTranslated(0, 0, .6);
   glRotated(spin, hitX, hitY, hitZ);
   gluSphere(quadric, .6, 20, 20);
   glEnable(GL_LIGHTING);
   glPopMatrix();
   glUseProgram(0);
}

void AsteroidShip::draw_ram() {
   float r1, g1, b1, r2, g2, b2;
   getBrightColor(color1, r1, g1, b1);
   getBrightColor(color2, r2, g2, b2);
   glUseProgram(ramShader);

   glPushMatrix();
   glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
      
      //floats used in loop iteration
      GLint loc1;
      GLint loc2;
      GLint loc3;
      GLint loc4;
      GLint loc5;
      GLint loc6;
      GLint loc7;

      glScaled(.3, .4, 1.1);
      glTranslated(0, 0, .5);

      if (flashiness > 100 || flashiness < -100) {
         upOrDown *= -1;
      }
      if (tracker > 75) {
            tracker = 0;
            rando = rand();
      }
      
      //printf("Flashiness: %f\n", flashiness);
      loc1 = glGetUniformLocation(ramShader,"poop");
      glUniform1f(loc1,flashiness);
      loc2 = glGetUniformLocation(ramShader,"r1");
      glUniform1f(loc2,r1);
      loc3 = glGetUniformLocation(ramShader,"g1");
      glUniform1f(loc3,g1);
      loc4 = glGetUniformLocation(ramShader,"b1");
      glUniform1f(loc4,b1);
      loc5 = glGetUniformLocation(ramShader,"r2");
      glUniform1f(loc5,r2);
      loc6 = glGetUniformLocation(ramShader,"g2");
      glUniform1f(loc6,g2);
      loc7 = glGetUniformLocation(ramShader,"b2");
      glUniform1f(loc7,b2);

      glDisable(GL_CULL_FACE);
      glEnable(GL_LIGHTING);
      glColor4f(1, 0, 0, 1);
      glBegin(GL_TRIANGLE_FAN);

    // Center of fan is at the origin
    glVertex3f(0.0f, 0.0f, -1.0);
    glVertex3f(1, 1, 1.1);
    glVertex3f(0, .8, 1);
    glVertex3f(-1, 1, 1.1);
    glVertex3f(-.8, 0, 1);
    glVertex3f(-1, -1, 1.1);
    glVertex3f(0, -.8, 1);
    glVertex3f(1, -1, 1.1);
    glVertex3f(.8, 0, 1);
    glVertex3f(1, 1, 1.1);

    glEnd();
    
    glBegin(GL_QUADS);
    glVertex3f(1, 1, 1.1);
    glVertex3f(0, .8, 1);
    glVertex3f(0, 0, .9);
    glVertex3f(.8, 0, 1);
    
    glVertex3f(-1, 1, 1.1);
    glVertex3f(-.8, 0, 1);
    glVertex3f(0, 0, .9);
    glVertex3f(0, .8, 1);
    
    glVertex3f(-1, -1, 1.1);
    glVertex3f(-.8, 0, 1);
    glVertex3f(0, 0, .9);
    glVertex3f(0, -.8, 1);
    
    glVertex3f(1, -1, 1.1);
    glVertex3f(.8, 0, 1);
    glVertex3f(0, 0, .9);
    glVertex3f(0, -.8, 1);
    glEnd();
      
      glEnable(GL_CULL_FACE);
   glLineWidth(1.0);
   glPopMatrix();
   glUseProgram(0);
   

}

bool AsteroidShip::isVulnerable() {
   if (health <= 0 || lives <= 0) {
      return false;
   }

   return (!(isRespawning() ||
      (isFiring && (currentWeapon == RAM_WEAPON_INDEX || gameState->godMode) 
       && weapons[RAM_WEAPON_INDEX]->isReady())));

}

void AsteroidShip::draw() {
   if (getHealth() <= 0 || lives <= 0)
      return;

   glPushMatrix();

   // Translate to the position.
   position->glTranslate();
   // Rotate to the current up/right/forward vectors.

   glRotate();
   
   spin+= 2;
   glColor4d(0, 0, 0, 1);

   if (drawSpawn && !(gameState->gsm == MenuMode)) {
      if (timeLeftToRespawn < 1.5) {
         glPushMatrix();
         draw_spawn();
         glPopMatrix();
      }
   } else if (isFirstSpawn && timeLeftToRespawn == -1) {
      timeLeftToRespawn = 1.5;
   } else {
      draw_ship();
      if(drawHit) {
         draw_hitEffect();
      }
      if (isFiring && (currentWeapon == RAM_WEAPON_INDEX || gameState->godMode)  && weapons[RAM_WEAPON_INDEX]->isReady()) {
         draw_ram();
      }
   }
   if (aliveTimer.getTimeRunning() > (6 * spawnRate)) {
      isFirstSpawn = false;
   }

   glPopMatrix();
}

void AsteroidShip::updateShotDirectionVector() {
   shotDirection.updateMagnitude(forward);
   shotDirection.rotate(shotPhi, *right);
   shotDirection.rotate(shotTheta, *up);
}

double AsteroidShip::getAimX() {
   return aimX;
}
double AsteroidShip::getAimY() {
   return aimY;
}

void AsteroidShip::debug() {
   printf("AsteroidShip::debug(): \n");
   minPosition->print();
   maxPosition->print();
   velocity->print();
   printf("--\n");
}

void AsteroidShip::updateShotDirection(Vector3D& dir) {
   shotDirection = dir;
}

Radar* AsteroidShip::getRadar() {
   return radar;
}

/**
 * This is the crosshair for first person.
 */
void AsteroidShip::drawCrosshair() {
   if (getHealth() <= 0 || timeLeftToRespawn > 0) {
      return;
   }

   // If we should be drawing the boxes, do that instead.
   if (currentView != VIEW_FIRSTPERSON_SHIP &&
         currentView != VIEW_FIRSTPERSON_GUN) {
      return drawShotDirectionIndicators();
   }

   double crosshairSizeX = 0.05;
   double thicknessX = 0.01;
   glPushMatrix();
   glLoadIdentity();

   // Make it white
   setMaterial(WhiteSolid);
   glColor3f(1, 1, 1);
   useOrtho();
   glDisable(GL_LIGHTING);

   if (currentView == VIEW_FIRSTPERSON_SHIP) {
      glTranslatef((GLfloat)getAimX(),(GLfloat) getAimY(),(GLfloat)0.0f);
   }
   static GLUquadricObj *outer;
   static GLUquadricObj *inner;
   outer = gluNewQuadric();
   gluDisk(outer, crosshairSizeX / 2 - thicknessX / 1.5, crosshairSizeX / 2, 12, 1);

   inner = gluNewQuadric();
   gluDisk(inner, 0.0, thicknessX / 1.5, 8, 1);

   glEnable(GL_LIGHTING);
   usePerspective();
   glPopMatrix();
}

/**
 * Draw the double crosshair, starfox style.
 */
void AsteroidShip::drawShotDirectionIndicators() {
   if (getHealth() <= 0)
      return;

   // Don't draw this while firing the tractorbeam.
   if (isFiring && (currentWeapon == TRACTOR_WEAPON_INDEX || gameState->godMode)) {
      return;
   }
   // The coords of the boxes.
   Point3D drawPoint = shotOrigin;
   double boxSize = shotOriginScale > 0 ? 1.0 : 0.6;
   const double boxDecrement = 0.15;
   const double distanceIncrement = shotOriginScale > 0 ? 5 : 1;

   Color boxColor(1.0f, 1.0f, 1.0f);
   Color hotColor(1.0f, 0, 0);
   Color midColor(1.0f, 0, 0);
   const int numBoxes = 6;
   
   double heatAmount = (1 - getCurrentWeaponCoolDown()) * numBoxes;

   double numHotBoxes = 0;
   double curFade = 0;

   bool overheated = getCurrentWeapon()->isOverheated();

   if (overheated) {
      curFade = (sin(gameState->getGameTime() * M_PI * 4) + 1) / 2;
   } else {
      curFade = modf(heatAmount, &numHotBoxes);
   }
   

   glPushMatrix();
   
   // Start at top right.
   up->movePoint(drawPoint, boxSize / 2);
   right->movePoint(drawPoint, boxSize / 2);
   glDisable(GL_LIGHTING);

   glLineWidth(3.0);


   for (int i = 0; i < numBoxes; ++i) {
      if (overheated) {
         hotColor.g = curFade;
         hotColor.b = curFade;
         hotColor.setColor();
      } else if (i < numHotBoxes) {
         hotColor.setColor();
      } else if (i == numHotBoxes) {
         midColor.g = 1 - curFade;
         midColor.b = 1 - curFade;
         midColor.setColor();
      } else {
         boxColor.setColor();
      }

      glBegin(GL_LINE_LOOP);
      // top right
      shotDirection.movePoint(drawPoint, distanceIncrement);
      drawPoint.draw();
      up->movePoint(drawPoint, -boxSize);
      drawPoint.draw();
      right->movePoint(drawPoint, -boxSize);
      drawPoint.draw();
      up->movePoint(drawPoint, boxSize);
      drawPoint.draw();
      glEnd();

      boxSize -= boxDecrement;
      right->movePoint(drawPoint, (boxDecrement / 2.0) + boxSize);
   }

   glLineWidth(1.0);
   glEnable(GL_LIGHTING);
   glEnable(GL_CULL_FACE);
   glPopMatrix();
}

/**
 * Returns a pointer to the currently selected weapon.
 */
Weapon* AsteroidShip::getCurrentWeapon() {
   return weapons[currentWeapon];
}

/*
 * Just tell us what the next weapon up would be, but don't switch to it.
 */
Weapon* AsteroidShip :: getNextWeapon() {
   //return weapons[(currentWeapon + 1) %weapons.size()];
   //int tmpIndex = min(currentWeapon + 1, (int)weapons.size() - 1);
   printf("getNext\n");
   int tmpIndex = max(currentWeapon, (currentWeapon + 1) % (int) weapons.size());
   return weapons[tmpIndex];
}


/*
 * Just tell us what the previous weapon would be, but don't switch to it.
 */
Weapon* AsteroidShip :: getPreviousWeapon() {
   //return weapons[(currentWeapon - 1) %weapons.size()];
   //int tmpIndex = max(currentWeapon - 1, 0);
   //int tmpIndex = min(currentWeapon - 1, (currentWeapon + (int) weapons.size() - 1) % (int) weapons.size());
   printf("getPrev\n");
   int tmpIndex = max(currentWeapon - 1, 0);
   return weapons[tmpIndex];
}

/**
 * Find the next available weapon.
 */
void AsteroidShip::nextWeapon() {
   selectWeapon(getNextWeaponID());
}

int AsteroidShip::getNextWeaponID() {
   int thisWeapon = (currentWeapon + 1) % weapons.size();
   while (thisWeapon != currentWeapon) {
      if (weapons[thisWeapon]->purchased) {
         return thisWeapon;
      } else {
         thisWeapon++;
      }
      if (thisWeapon >= (int) weapons.size()) {
         thisWeapon = 0;
      }
   }
   return thisWeapon;
}

/**
 * Find the previous available weapon.
 */
void AsteroidShip::prevWeapon() {
   selectWeapon(getPrevWeaponID());
}

int AsteroidShip::getPrevWeaponID() {
   int thisWeapon = (weapons.size() + currentWeapon - 1) % weapons.size();
   while (thisWeapon != currentWeapon) {
      if (weapons[thisWeapon]->purchased) {
         return thisWeapon;
      } else {
         thisWeapon--;
      }
      if (thisWeapon <= 0) {
         thisWeapon = weapons.size() - 1;
      }
   }
   return thisWeapon;
}

// Get the number of types of weapons the ship has. They're indexed 0 - (n-1)
int AsteroidShip :: getNumWeapons() {
   return (int) weapons.size();
}

/**
 * Gets the cooldownamount of the currently selected weapon.
 */
float AsteroidShip::getCurrentWeaponCoolDown() {
   return (float) weapons[currentWeapon]->getCoolDownAmount();
}

Weapon* AsteroidShip::getWeapon(int wep) {
   return weapons[wep];
}

float AsteroidShip::getShakeAmount() {
   return shakeAmount;
}

/**
 * How long has the ship been alive for?
 */
double AsteroidShip::getAliveTime() {
   return aliveTimer.getTimeRunning();
}

void AsteroidShip::setShakeAmount(float shakeIn) {
   shakeAmount = shakeIn;
}

Vector3D* AsteroidShip::getViewVector() {
   switch(currentView) {
   case VIEW_FIRSTPERSON_SHIP:
   case VIEW_THIRDPERSON_SHIP: return forward;
   case VIEW_FIRSTPERSON_GUN:
   case VIEW_THIRDPERSON_GUN: return getShotDirection();
   default:
                              fprintf(stderr, "getViewVector got currentView %d\n", currentView);
                              return forward;
   }
}

Vector3D* AsteroidShip::getCameraOffset() {
   //const double thirdPersonForwardOffset = -12;
   const double thirdPersonForwardOffset = -6;
   // This is in spring.
   const double thirdPersonUpOffset = 0;
   const double firstPersonForwardOffset = 0.5 * shotOriginScale;
   const double firstPersonUpOffset = 0.0; //0.5;
   if (currentView == VIEW_THIRDPERSON_SHIP || currentView == VIEW_THIRDPERSON_GUN) {
      cameraOffset->updateMagnitude(getViewVector()->scalarMultiply(thirdPersonForwardOffset));
      cameraOffset->addUpdate(up->scalarMultiply(thirdPersonUpOffset));
      cameraOffset->scalarMultiplyUpdate(zoomFactor);
   } else {
      cameraOffset->updateMagnitude(forward->scalarMultiply(firstPersonForwardOffset));
      cameraOffset->addUpdate(up->scalarMultiply(firstPersonUpOffset));
   }
   return cameraOffset;
}

void AsteroidShip::nextView() {
   if (currentView == VIEW_FIRSTPERSON_SHIP)
      currentView = VIEW_THIRDPERSON_SHIP;
   else
      currentView = VIEW_FIRSTPERSON_SHIP;
}

void AsteroidShip::setView(int _view) {
   if (_view > VIEW_COUNT) {
      fprintf(stderr, "setView got currentView %d\n", currentView);
      _view %= VIEW_COUNT;
   }
   currentView = _view;
}

int AsteroidShip::getCurrentView() {
   return currentView;
}

/**
 * Used by Radar to move the camera to the shot origin, so that VFC for
 * the ShootingAI will only target things it could see.
 */
void AsteroidShip::setCameraToShotOrigin() {
   gluLookAt(shotOrigin.x, shotOrigin.y, shotOrigin.z,
         shotOrigin.x + forward->x,
         shotOrigin.y + forward->y,
         shotOrigin.z + forward->z,
         up->x, up->y, up->z);
}

void AsteroidShip::setZoomSpeed(float speed) {
   zoomSpeed = speed;
}

/**
 * This is used to set up ships initially.
 * Do not use this for moving ships in the game.
 * The vectors should come out as orthogonal.
 * This sets right to be forward x up.
 * Then up is set to right x forward.
 */
void AsteroidShip::setOrientation(double forwardX, double forwardY, double forwardZ,
      double upX, double upY, double upZ) {
   // Set forward and
   forward->updateMagnitude(forwardX, forwardY, forwardZ);
   up->updateMagnitude(upX, upY, upZ);

   forward->normalize();
   up->normalize();

   right->updateMagnitude(forward->cross(*up));
   up->updateMagnitude(right->cross(*forward));
}

/**
 * Same as the other setOrientation.
 */
void AsteroidShip::setOrientation(Vector3D& forward, Vector3D& up) {
   setOrientation(forward.x, forward.y, forward.z,
         up.x, up.y, up.z);
}

/**
 * Look at some point with an arbitrary up vector.
 * This is used by the menu, and should not be used in the game.
 */
void AsteroidShip::lookAt(double lookAtX, double lookAtY, double lookAtZ,
      double upX, double upY, double upZ) {
   Vector3D _forward(lookAtX - position->x,
         lookAtY - position->y,
         lookAtZ - position->z);

   Vector3D _up(upX, upY, upZ);

   setOrientation(_forward, _up);
}

bool AsteroidShip::isRespawning() {
   //printf("Is it respawning? : %d\n", respawnTimer.isRunning && (respawnTimer.getTimeLeft() + spawnRate) > 0);
   return lives <= 0 || 
    (respawnTimer.isRunning && respawnTimer.getTimeLeft() > 0
     && timeLeftToRespawn > 0);
}

Shard* AsteroidShip::makeShard() {
   Vector3D randomOffset;
   randomOffset.randomMagnitude();
   randomOffset.scalarMultiplyUpdate(3);

   Shard* shard;
   shard = new Shard(0.5, gameState->worldSize, gameState);
   shard->position->clone(position);
   randomOffset.movePoint(*shard->position);
   shard->velocity->updateMagnitude(position, shard->position);
   shard->velocity->scalarMultiplyUpdate(8 * randdouble());
   return shard;
}

void AsteroidShip::readCommand(const ast::ClientCommand& command) {
   accelerateForward(command.forwardacceleration());
   //accelerateRight(command.rightAcceleration);
   //accelerateUp(command.upacceleration());

   setYawSpeed(command.yawspeed());
   setRollSpeed(command.rollspeed());
   setPitchSpeed(command.pitchspeed());

   fire(command.fire());

   setBrake(command.brake());

   selectWeapon(command.curweapon());

   // This works as long as VERT_FOV is the same on both sides.
   updateShotDirection(command.mousex(), command.mousey());

   if (command.rightacceleration() == -1 && isBarrelRollingLeft < 0 && isBarrelRollingRight < 0){
      isBarrelRollingLeft = 1;
   }
   
   if (command.rightacceleration() == 1 && isBarrelRollingLeft < 0 && isBarrelRollingRight < 0) {
      isBarrelRollingRight = 1;
   }

   if (command.has_name()) {
      name = command.name();
   }
}

/**
 * This is called on the ship when the level ends.
 */
void AsteroidShip::atLevelEnd() {
   /*
   if (this == gameState->ship && unbankedShards > 0) {
      SoundEffect::playSoundEffect("ShardBank", NULL, NULL, true, 0.8f);
   }
   */
   if (gameState->gsm != ClientMode) {
      bankedShards += unbankedShards;
      totalBankedShards += unbankedShards;

      unbankedShards = 0;
      health = healthMax;
      
      respawnTimer.reset();
      timeLeftToRespawn = -1;
   }
   shakeAmount = 0;

   if (gameState->gsm == ServerMode) {
      // Give Player one more life at each level.
      lives++;
   }

   stopSounds();
}

/**
 * Called when the player exits the Store Menu.
 * It purchases a new weapon for the player based on what level they're at,
 * and displays a text message on screen telling them it's available.
 */
void AsteroidShip::unlockWeapons() {
   switch(gameState->curLevel) {
      case 2:
         // Buy the Homing Missile gun on level 2.
         getWeapon(HOMINGMISSILE_WEAPON_INDEX)->purchased = true;
         gameState->addWeaponUnlockMessage(getWeapon(HOMINGMISSILE_WEAPON_INDEX));
         break;
      case 3:
         // Buy the Lightning gun on level 3.
         getWeapon(ELECTRICITY_WEAPON_INDEX)->purchased = true;
         gameState->addWeaponUnlockMessage(getWeapon(ELECTRICITY_WEAPON_INDEX));
         break;
      case 4:
         // Buy the Ram on level 4.
         getWeapon(RAM_WEAPON_INDEX)->purchased = true;
         gameState->addWeaponUnlockMessage(getWeapon(RAM_WEAPON_INDEX));
         break;
      case 5:
         // Buy the Timed Bomb on level 5.
         getWeapon(TIMEDBOMBER_WEAPON_INDEX)->purchased = true;
         gameState->addWeaponUnlockMessage(getWeapon(TIMEDBOMBER_WEAPON_INDEX));
         break;
      case 6:
         // Buy the Charge Cannon on level 5.
         getWeapon(ENERGY_WEAPON_INDEX)->purchased = true;
         gameState->addWeaponUnlockMessage(getWeapon(ENERGY_WEAPON_INDEX));
         break;
      case 7:
         // Buy the Railgun on level 5.
         getWeapon(RAILGUN_WEAPON_INDEX)->purchased = true;
         gameState->addWeaponUnlockMessage(getWeapon(RAILGUN_WEAPON_INDEX));
         break;

   }

}

void AsteroidShip::stopSounds() {
   if (soundHandle != NULL) {
      SoundEffect::stopSoundEffect(soundHandle);
      soundHandle = NULL;
   }

   for (unsigned i = 0; i < weapons.size(); ++i) {
      weapons[i]->stopSounds();
   }
}

// Price functions
int AsteroidShip::healthMaxUpgradePrice() { 
   return 5;
}

int AsteroidShip::regenUpgradePrice() { 
   return (int)pow(1.6,regenHealthLevel); 
}

int AsteroidShip::engineUpgradePrice() { 
   return 10*engineLevel; 
}

int AsteroidShip::bankUpgradePrice() { 
   return 2*bankLevel;
}

int AsteroidShip::lifePrice() { 
   return 10 * floor((double)gameState->curLevel + (double)(1/3)); 
}

// Upgrade functions
void AsteroidShip::healthMaxUpgrade() {
   bankedShards -= healthMaxUpgradePrice();
   healthMax += 10;
}

void AsteroidShip::regenUpgrade() {
   bankedShards -= regenUpgradePrice();
   regenHealthLevel += 1;
}

void AsteroidShip::engineUpgrade() {
   bankedShards -= engineUpgradePrice();
   engineLevel += 1;
}

void AsteroidShip::bankUpgrade() {
   bankedShards -= bankUpgradePrice();
   bankLevel += 1;
}

void AsteroidShip::buyLife() {
   bankedShards -= lifePrice();
   lives += 1;
}

// calculates the correct bank period depending on the bankLevel
double AsteroidShip::getBankPeriod() {
   return ((double)bankPeriod) / ((double)bankLevel+1);
}

bool AsteroidShip::saveDiff(const ast::Entity& old, ast::Entity* ent) {
   bool changed = Object3D::saveDiff(old, ent);
   
   /*
   if (!up->saveDiff(old.up(), ent->mutable_up())) {
      ent->clear_up();
   } else {
      changed = true;
   }
   
   if (!forward->saveDiff(old.forward(), ent->mutable_forward())) {
      ent->clear_forward();
   } else {
      changed = true;
   }
   */
   
   /*
   if (!right->saveDiff(old.right(), ent->mutable_right()))
      ent->clear_right();
   else
      changed = true;
      */
   
   if (!up->saveDiff(old.up(), ent->mutable_targetup())) {
      ent->clear_targetup();
   } else {
      changed = true;
   }
   
   if (!forward->saveDiff(old.forward(), ent->mutable_targetforward())) {
      ent->clear_targetforward();
   } else {
      changed = true;
   }
   
   
   if (!shotDirection.saveDiff(old.shotdirection(), ent->mutable_shotdirection())) {
      ent->clear_shotdirection();
   } else {
      changed = true;
   }

   if (!bankTimer.saveDiff(old.banktimer(), ent->mutable_banktimer())) {
      ent->clear_banktimer();
   }
   if (!aliveTimer.saveDiff(old.alivetimer(), ent->mutable_alivetimer())) {
      ent->clear_alivetimer();
   }
   if (!respawnTimer.saveDiff(old.respawntimer(), ent->mutable_respawntimer())) {
      ent->clear_respawntimer();
   }

   // Handle weapons.
   for (int i = 0; i < NUMBER_OF_WEAPONS; ++i) {
      if (!getWeapon(i)->saveDiff(old.weapon(i), ent->add_weapon()))
         ent->mutable_weapon()->RemoveLast();
      else {
         changed = true;
         //debugoutput << "weap " << i << " changed.\n";
      }
   }

   if (name.compare(old.name()) != 0) {
      ent->set_name(name);
      changed = true;
      // Maybe print name change message?
   }

   if (killedBy != old.killedby()) {
      ent->set_killedby(killedBy);
      changed = true;
   }
   
   if (lastDamagerId != old.lastdamagerid()) {
      ent->set_lastdamagerid(lastDamagerId);
      changed = true;
   }
   
   if (lastDamagerWeapon != old.lastdamagerweapon()) {
      ent->set_lastdamagerweapon(lastDamagerWeapon);
      changed = true;
   }

   if (targetRollSpeed != old.targetrollspeed()) {
      ent->set_targetrollspeed(targetRollSpeed);
      changed = true;
   }

   if (targetYawSpeed != old.targetyawspeed()) {
      ent->set_targetyawspeed(targetYawSpeed);
      changed = true;
   }

   if (targetPitchSpeed != old.targetpitchspeed()) {
      ent->set_targetpitchspeed(targetPitchSpeed);
      changed = true;
   }


   if (health != old.health()) {
      ent->set_health(health);
      changed = true;
   }

   if (healthMax != old.healthmax()) {
      ent->set_healthmax(healthMax);
      changed = true;
   }

   
   if (engineLevel != old.enginelevel()) {
      ent->set_enginelevel(engineLevel);
      changed = true;
   }

   if (regenHealthLevel != old.regenhealthlevel()) {
      ent->set_regenhealthlevel(regenHealthLevel);
      changed = true;
   }

   if (bankLevel != old.banklevel()) {
      ent->set_banklevel(bankLevel);
      changed = true;
   }

   if (color1 != old.color1()) {
      ent->set_color1(color1);
      changed = true;
   }

   if (color2 != old.color2()) {
      ent->set_color2(color2);
      changed = true;
   }

   
   if (isFiring != old.isfiring()) {
      ent->set_isfiring(isFiring);
      changed = true;
   }
   
   if (currentWeapon != old.currentweapon()) {
      ent->set_currentweapon(currentWeapon);
      changed = true;
   }
   
   if (isBarrelRollingLeft != old.isbarrelrollingleft()) {
      ent->set_isbarrelrollingleft(isBarrelRollingLeft);
      changed = true;
   }
   
   if (isBarrelRollingRight != old.isbarrelrollingright()) {
      ent->set_isbarrelrollingright(isBarrelRollingRight);
      changed = true;
   }
   
   if (curForwardAccel != old.curforwardaccel()) {
      ent->set_curforwardaccel(curForwardAccel);
      changed = true;
   }
   
   if (curUpAccel != old.curupaccel()) {
      ent->set_curupaccel(curUpAccel);
      changed = true;
   }
   
   if (curRightAccel != old.currightaccel()) {
      ent->set_currightaccel(curRightAccel);
      changed = true;
   }


   
   if (bankPeriod != old.bankperiod()) {
      ent->set_bankperiod(bankPeriod);
      changed = true;
   }

   
   if (flyingAI->isEnabled() != old.flyingaienabled()) {
      ent->set_flyingaienabled(flyingAI->isEnabled());
      changed = true;
   }
   
   if (shooter->isEnabled() != old.shootingaienabled()) {
      ent->set_shootingaienabled(shooter->isEnabled());
      changed = true;
   }

   
   if (timeLeftToRespawn != old.timelefttorespawn()) {
      ent->set_timelefttorespawn(timeLeftToRespawn);
      changed = true;
   }
   
   if (score != old.score()) {
      ent->set_score(score);
      changed = true;
   }

   if (kills != old.kills()) {
      ent->set_kills(kills);
      changed = true;
   }

   if (deaths != old.deaths()) {
      ent->set_deaths(deaths);
      changed = true;
   }

   if (lives != old.lives()) {
      ent->set_lives(lives);
      changed = true;
   }

   if (bankedShards != old.bankedshards()) {
      ent->set_bankedshards(bankedShards);
      changed = true;
   }

   if (unbankedShards != old.unbankedshards()) {
      ent->set_unbankedshards(unbankedShards);
      changed = true;
   }

   if (totalBankedShards != old.totalbankedshards()) {
      ent->set_totalbankedshards(totalBankedShards);
      changed = true;
   }


   return changed;
}

void AsteroidShip::save(ast::Entity* ent) {
   Object3D::save(ent);
   // Only save two of these and calc the third.
   up->save(ent->mutable_up());
   forward->save(ent->mutable_forward());
   // Don't save right.
   
   shotDirection.save(ent->mutable_shotdirection());
   
   bankTimer.save(ent->mutable_banktimer());
   aliveTimer.save(ent->mutable_alivetimer());
   respawnTimer.save(ent->mutable_respawntimer());

   
   ast::Weapon* weap;
   for (int i = 0; i < NUMBER_OF_WEAPONS; ++i) {
      weap = ent->add_weapon();
      getWeapon(i)->save(weap);
   }

   ent->set_name(name);
      
   ent->set_killedby(killedBy);
   ent->set_lastdamagerid(lastDamagerId);
   ent->set_lastdamagerweapon(lastDamagerWeapon);

   ent->set_targetrollspeed(targetRollSpeed);
   ent->set_targetyawspeed(targetYawSpeed);
   ent->set_targetpitchspeed(targetPitchSpeed);

   ent->set_health(health);
   ent->set_healthmax(healthMax);

   ent->set_enginelevel(engineLevel);
   ent->set_regenhealthlevel(regenHealthLevel);
   ent->set_banklevel(bankLevel);
   ent->set_color1(color1);
   ent->set_color2(color2);
   
   ent->set_isfiring(isFiring);
   ent->set_currentweapon(currentWeapon);
   ent->set_isbarrelrollingleft(isBarrelRollingLeft);
   ent->set_isbarrelrollingright(isBarrelRollingRight);
   ent->set_curforwardaccel(curForwardAccel);
   ent->set_curupaccel(curUpAccel);
   ent->set_currightaccel(curRightAccel);

   ent->set_bankperiod(bankPeriod);

   ent->set_flyingaienabled(flyingAI->isEnabled());
   ent->set_shootingaienabled(shooter->isEnabled());

   ent->set_timelefttorespawn(timeLeftToRespawn);

   ent->set_score(score);
   ent->set_kills(kills);
   ent->set_deaths(deaths);
   ent->set_lives(lives);
   ent->set_bankedshards(bankedShards);
   ent->set_unbankedshards(unbankedShards);
   ent->set_totalbankedshards(totalBankedShards);
}

void AsteroidShip::load(const ast::Entity& ent) {
   Object3D::load(ent);

   // Load weapons.
   for (int i = 0; i < ent.weapon_size(); ++i) {
      const ast::Weapon& weap = ent.weapon(i);
      unsigned index = weap.index();
      getWeapon(index)->load(weap);
   }

   // When killedby is set by the server, deathAcknowledged is set to false.
   // We only do this when the local killedBy is 0 (meaning player is alive 
   // and just getting killed) or when ent.killedby() is 0, meaning player 
   // is getting reset right now.
   // We don't set deathAcknowledged to false when getting reset, though.
   if (ent.has_killedby() && (killedBy == KILLEDBY_DEFAULT || ent.killedby() == KILLEDBY_DEFAULT) &&
    killedBy != ent.killedby()) {
      killedBy = ent.killedby();
      // DEBUG
      cout << "Setting killedby to " << killedBy << " for " << name << endl;
      if (killedBy != KILLEDBY_DEFAULT) {
         deathAcknowledged = false;
      }
   }

   if (ent.has_lastdamagerid()) {
      lastDamagerId = ent.lastdamagerid();
   }
   
   if (ent.has_lastdamagerweapon()) {
      lastDamagerWeapon = ent.lastdamagerweapon();
   }

   if (ent.has_name()) {
      name = ent.name();
   }

   if (ent.has_forward() || ent.has_up()) {
      printf("got original fwd / up\n");
      if (ent.has_forward())
         forward->load(ent.forward());
      if (ent.has_up())
         up->load(ent.up());

      forward->normalize();
      up->normalize();
      *right = forward->cross(*up);

      targetForward = *forward;
      targetUp = *up;
      targetRight = *right;

      interpolateOrientation = false;
      orientationInterpolationAmount = 0;
      
   }

   if (ent.has_targetforward() || ent.has_targetup()) {
      if (ent.has_targetforward())
         targetForward.load(ent.targetforward());
      if (ent.has_targetup())
         targetUp.load(ent.targetup());

      targetForward.normalize();
      targetUp.normalize();
      targetRight = targetForward.cross(targetUp);
      interpolateOrientation = true;
      orientationInterpolationAmount = 1;
   }

   if (ent.has_health())
      health = ent.health();

   if (ent.has_healthmax())
      healthMax = ent.healthmax();

   if (ent.has_targetrollspeed())
      targetRollSpeed = ent.targetrollspeed();
   if (ent.has_targetyawspeed())
      targetYawSpeed = ent.targetyawspeed();
   if (ent.has_targetpitchspeed())
      targetPitchSpeed = ent.targetpitchspeed();

   if (ent.has_enginelevel())
      engineLevel = ent.enginelevel();

   if (ent.has_regenhealthlevel())
      regenHealthLevel = ent.regenhealthlevel();

   if (ent.has_banklevel())
      bankLevel = ent.banklevel();

   if (ent.has_color1())
      color1 = ent.color1();

   if (ent.has_color2())
      color2 = ent.color2();

   // We don't want to update shot direction for ourself because it causes choppy animation.
   if (gameState->gsm != ClientMode && this != gameState->ship) {
      if (ent.has_shotdirection())
         shotDirection.load(ent.shotdirection());
   }

   if (ent.has_isfiring())
      isFiring = ent.isfiring();

   if (ent.has_currentweapon())
      currentWeapon = ent.currentweapon();

   if (ent.has_isbarrelrollingleft())
      isBarrelRollingLeft = ent.isbarrelrollingleft();
   if (ent.has_isbarrelrollingright())
      isBarrelRollingRight = ent.isbarrelrollingright();

   if (ent.has_curforwardaccel())
      curForwardAccel = ent.curforwardaccel();
   if (ent.has_curupaccel())
      curUpAccel = ent.curupaccel();
   if (ent.has_currightaccel())
      curRightAccel = ent.currightaccel();

   if (ent.has_isbraking())
      isBraking = ent.isbraking();

   if (ent.has_bankperiod())
      bankPeriod = ent.bankperiod();

   if (ent.flyingaienabled())
      flyingAI->enable();
   
   if (ent.shootingaienabled())
      shooter->enable();

   if (ent.has_banktimer())
      bankTimer.load(ent.banktimer());
   if (ent.has_alivetimer())
      aliveTimer.load(ent.alivetimer());
   if (ent.has_respawntimer())
      respawnTimer.load(ent.respawntimer());

   if (ent.has_timelefttorespawn())
      timeLeftToRespawn = ent.timelefttorespawn();

   if (ent.has_score())
      score = ent.score();
   if (ent.has_kills())
      kills = ent.kills();
   if (ent.has_deaths())
      deaths = ent.deaths();
   if (ent.has_lives())
      lives = ent.lives();
   if (ent.has_bankedshards())
      bankedShards = ent.bankedshards();
   if (ent.has_unbankedshards())
      unbankedShards = ent.unbankedshards();
   if (ent.has_totalbankedshards())
      totalBankedShards = ent.totalbankedshards();
}

void AsteroidShip::createExplosionParticles() {
   // Make some sparkles when you die!~~~
   for (int i = 0; i < 500; ++i) {
      Point3D* particleStartPoint = new Point3D(*position);
      Vector3D* particleDirection = new Vector3D();
      particleDirection->randomMagnitude();
      particleDirection->setLength(10 * randdouble());
      particleDirection->addUpdate(velocity->scalarMultiply(randdouble()));
      ElectricityImpactParticle::Add(particleStartPoint, particleDirection, gameState);
   }
}

void AsteroidShip::onRemove() {
   Object3D::onRemove();
   createExplosionParticles();
}

void AsteroidShip::dropShards() {
   Shard* tmp;
   while (unbankedShards > 0) {
      custodian->add(makeShard());
      --unbankedShards;
   }

   // Make a few more for good measure.
   do {
      tmp = makeShard();
      custodian->add(tmp);
   } while(rand() % 2 == 0);

   if (gameState->gsm == ServerMode) {
      while (healthMax > 100) {
         tmp = makeShard();
         tmp->shardType = SHARD_TYPE_MAXHEALTH;
         custodian->add(tmp);
         healthMax -= 10;
      }

      while (engineLevel > 1) {
         tmp = makeShard();
         tmp->shardType = SHARD_TYPE_ENGINE;
         custodian->add(tmp);
         --engineLevel;
      }

      while (regenHealthLevel > 0) {
         tmp = makeShard();
         tmp->shardType = SHARD_TYPE_REGEN;
         custodian->add(tmp);
         --regenHealthLevel;
      }

      Weapon* weap;
      for (int i = 0; i < NUMBER_OF_WEAPONS; ++i) {
         weap = getWeapon(i); 
         if (weap->purchased) {
            weap->level = 1;
            /*
               while (weap->level > 1) {
               tmp = makeShard();
               tmp->shardType = SHARD_TYPE_WEAPON;
               tmp->weapNum = i;
               custodian->add(tmp);
               --weap->level;

               }
             */

            if (i > BLASTER_WEAPON_INDEX) {
               tmp = makeShard();
               tmp->shardType = SHARD_TYPE_WEAPON;
               tmp->weapNum = i;
               custodian->add(tmp);

               weap->purchased = false;
            }
         }
      }
   }
}

void AsteroidShip::randomizePosition() {
   if (gameState->gsm != ClientMode) {
      double worldSize = gameState->worldSize - 4; // Give some space.
      double randX = (randdouble())*(worldSize / 2);
      double randY = (randdouble())*(worldSize / 2);
      double randZ = (randdouble())*(worldSize / 2);
      position->update(randX, randY, randZ);
      forward->updateMagnitude(-position->x, -position->y, -position->z);
      forward->normalize();
      up->updateMagnitude(forward->getNormalVector());
      up->normalize();
      right->updateMagnitude(forward->cross(*up));
      right->normalize();
   }
   
   shotDirection.updateMagnitude(0, 0, 1);
   shotOrigin = *position;
   forward->movePoint(shotOrigin, shotOriginScale);
}

bool AsteroidShip::isFullAIEnabled() {
   return shooter->isEnabled() && flyingAI->isEnabled();
}

/**
 * Handle respawning.
 */
void AsteroidShip::doRespawn(double timeDiff) {
   deathAcknowledged = true;
   if (this == gameState->ship) {
      gameState->usingShipCamera = false;
   }

   if (gameState->gsm != ClientMode) {
      if (lives > 0) {
         // We want this to happen if a user has 1 lives left, I guess.
         ++deaths;
         --lives;
      }

      // Since lives changed, let's check again.
      if (lives > 0) {
         respawnTimer.setCountDown(RESPAWN_TIME);
         timeLeftToRespawn = respawnTimer.getTimeLeft();
         
         // Make sure to stop barrel rolling.
         isBarrelRollingLeft = -1;
         isBarrelRollingRight = -1;
      
      } else {
         // No more lives, just stay quiet.
         // Update weapons one last time.
         for (vector<Weapon*>::iterator iter = weapons.begin();
               iter != weapons.end(); ++iter) {
            (*iter)->update(timeDiff);
         }
         if (this == gameState->ship) {
            if (gameState->gsm == SingleMode) {
               gameState->gameOver();
            }
         } else {
            if (gameState->gsm == SingleMode || 
              shooter->isEnabled() || flyingAI->isEnabled()) {
               shouldRemove = true;
            }
         }
      }

      killedBy = lastDamagerId;
   }

   addKillMessage();

   // Update weapons one last time.
   for (vector<Weapon*>::iterator iter = weapons.begin();
         iter != weapons.end(); ++iter) {
      (*iter)->update(timeDiff);
   }

   // Fix all the velocities with anything added from the killer.
   Object3D::update(timeDiff);       

   createExplosionParticles();
   // Release all the shards.
   if (gameState->gsm != ClientMode) {
      dropShards();
   }
}

void AsteroidShip::zeroControls() {
   // Set dead state.
   shouldDrawInMinimap = false;
   fire(false);
   setRollSpeed(0);
   accelerateForward(0);
   accelerateUp(0);
   accelerateRight(0);
   setYawSpeed(0.0);
   setPitchSpeed(0.0);
   setRollSpeed(0.0);
}

void AsteroidShip::doDeadStuff(double timeDiff) {
   if (timeDiff <= 0)
      return;

   zeroControls();
   shakeAmount = 0;

   stopSounds();

   if (this == gameState->ship) {
      gameState->usingShipCamera = false;
   }

   if ((gameState->gsm != ClientMode && !isRespawning()) ||
    (gameState->gsm == ClientMode && !deathAcknowledged)) {
      doRespawn(timeDiff);
   } else {
      // This is set in doRespawn()
      timeLeftToRespawn = respawnTimer.getTimeLeft();
   }

   if (this == gameState->ship && lives > 0) {
      addRespawnMessage();
   }
      
   if (gameState->gameIsRunning && respawnTimer.isRunning && 
    timeLeftToRespawn <= 1.5 && lives > 0) {
      timeLeftToRespawn = 1.5;
      // This cancels the respawn timer.
      reInitialize();
   }
}

void AsteroidShip::doRegenHealth(double timeDiff) {
   health += regenHealthLevel * timeDiff;
   if (health > healthMax) {
      health = healthMax;
   }
}

void AsteroidShip::doBraking(double timeDiff) {
   if (isBraking) {
      if (velocity->getComparisonLength() < 0.01) {
         velocity->updateMagnitude(0, 0, 0);
      } else {
         velocity->x -= velocity->x * timeDiff * brakeFactor;
         velocity->y -= velocity->y * timeDiff * brakeFactor;
         velocity->z -= velocity->z * timeDiff * brakeFactor;
      }
   }
}

void AsteroidShip::limitVelocity(double timeDiff) {
   if (velocity->getComparisonLength() > 
    (maxSpeed + engineLevel * 2) * (maxSpeed + engineLevel * 2)) {
      Vector3D* slowDown = new Vector3D(velocity->scalarMultiply(-0.8 * timeDiff));
      velocity->addUpdate(slowDown);
   }  
}

void AsteroidShip::applyRotationalAcceleration(double timeDiff) {
   if (rollSpeed > targetRollSpeed) {
      rollSpeed = clamp(rollSpeed - (timeDiff * rotationalAcceleration),
            targetRollSpeed, rollSpeed);
   } else if (rollSpeed < targetRollSpeed) {
      rollSpeed = clamp(rollSpeed + (timeDiff * rotationalAcceleration),
            rollSpeed, targetRollSpeed);
   }

   if (pitchSpeed > targetPitchSpeed) {
      pitchSpeed = clamp(pitchSpeed - (timeDiff * rotationalAcceleration),
            targetPitchSpeed, pitchSpeed);
   } else if (pitchSpeed < targetPitchSpeed) {
      pitchSpeed = clamp(pitchSpeed + (timeDiff * rotationalAcceleration),
            pitchSpeed, targetPitchSpeed);
   }

   if (yawSpeed > targetYawSpeed) {
      yawSpeed = clamp(yawSpeed - (timeDiff * rotationalAcceleration),
            targetYawSpeed, yawSpeed);
   } else if (yawSpeed < targetYawSpeed) {
      yawSpeed = clamp(yawSpeed + (timeDiff * rotationalAcceleration),
            yawSpeed, targetYawSpeed);
   }
   
   if (isBarrelRollingLeft <= 0 && isBarrelRollingRight <= 0) {
      roll(timeDiff * rollSpeed);
      pitch(timeDiff * pitchSpeed);
      yaw(timeDiff * yawSpeed);
   }
}

void AsteroidShip::doInterpolateOrientation(double timeDiff) {
   if (interpolateOrientation && orientationInterpolationAmount >= 0) {
      // Stop interpolating if we don't have further to go.
      if (orientationInterpolationAmount == 0)
         interpolateOrientation = false;

      float curFrameInterpolation = 1 - orientationInterpolationAmount;
      *up = up->lerp(targetUp, curFrameInterpolation);
      *forward = forward->lerp(targetForward, curFrameInterpolation);
      up->normalize();
      forward->normalize();
      *right = forward->cross(*up);
      right->normalize();

      orientationInterpolationAmount -= timeDiff * 10; // Interpolate over .1 sec

      if (orientationInterpolationAmount < 0)
         orientationInterpolationAmount = 0;
   }
}

void AsteroidShip::bankShards() {
   bool showBankedShardsMessage = false;
   // Bank shards.
   if (bankTimer.isRunning) {
      if (unbankedShards <= 0) {
         // This is awkward. Recover silently.
         bankTimer.reset();
      } else if (bankTimer.getTimeLeft() < 0) {
         bankTimer.reset();
         if (gameState->gsm != ClientMode) {
            bankedShards++;
            totalBankedShards++;
            unbankedShards--;
         }

         // Play the sound effect only sometimes.
         // Something else special should happen.
         ostringstream sstream;

         switch (totalBankedShards) {
            case 1: case 5: case 10: case 25: case 50:
               showBankedShardsMessage = true;
               break;
            default:
               if (totalBankedShards % 100 == 0) {
                  showBankedShardsMessage = true;
               }
         }
         
         if (gameState->ship == this && showBankedShardsMessage) {
            SoundEffect::playSoundEffect("ShardBank", NULL, NULL, true, 0.3f);
            if (totalBankedShards == 1) {
               sstream << "Banked first shard!";
            } else {
               sstream << "Banked " << totalBankedShards << " total shards!";
            }

            GameMessage::Add(sstream.str(), 1, 2, gameState);
         }
      }
   } else if (unbankedShards > 0 && gameState->gsm != ClientMode) {
      bankTimer.setCountDown(getBankPeriod());
   }
}

void AsteroidShip::updateWeapons(double timeDiff) {
   for (vector<Weapon*>::iterator iter = weapons.begin();
         iter != weapons.end(); ++iter) {
      (*iter)->update(timeDiff);
   }
}

void AsteroidShip::updateSound() {
   if ((gameState->gsm != MenuMode) && (storeMenu != NULL) && !storeMenu->menuActive &&
         (curForwardAccel != 0 || curUpAccel != 0 || curRightAccel != 0)) {
      if (soundHandle == NULL) {
         soundHandle = SoundEffect::playSoundEffect("ShipEngine.wav",
               position, velocity, (this == gameState->ship), 0.05f, true);
      } else {
         SoundEffect::updateSource(soundHandle, position, velocity);
      }
   } else {
      if (soundHandle != NULL) {
         SoundEffect::stopSoundEffect(soundHandle);
         soundHandle = NULL;
      }
   }
}

void AsteroidShip::reduceShake(double timeDiff) {
   if (shakeAmount != 0) {
      shakeAmount -= (float) (5 * shakeAmount * timeDiff);
      if (shakeAmount < 0.01) {
         shakeAmount = 0;
      }
   }
}

void AsteroidShip::updateSpaceBoner(double timeDiff) {
   if (curForwardAccel == 10) {
      backChange += (zMove * timeDiff);
      if (backChange > (backZ - middleZ)) {
         backChange = (backZ - middleZ);
      }

      if (backChange == (backZ - middleZ)) {
         xChange += lineMove * timeDiff;
         yChange += lineMove * timeDiff;
         zChange += zMove * timeDiff;
         x2Change += lineMove * timeDiff;
         y2Change += lineMove * timeDiff;
         z2Change += zMove * timeDiff;
      }

      if (x2Change > middleXY) {
         xChange = 0;
         yChange = 0;
         zChange = 0;
         x2Change = middleXY / 2;
         y2Change = middleXY / 2;
         z2Change = (backZ - middleZ) / 2;
      }

   } else {
      backChange -= zMove * timeDiff;
      if (backChange < 0) {
         backChange = 0;
      }
   }
}

void AsteroidShip::handleBarrelRoll(double timeDiff) {
      if (gameState->gsm != ClientMode) {
         if (isBarrelRollingLeft == 1)
            addInstantAcceleration(new Vector3D(right->scalarMultiply(-20)));
         if (isBarrelRollingRight == 1)
            addInstantAcceleration(new Vector3D(right->scalarMultiply(20)));
      }

      // Do this client side too.
      if (isBarrelRollingLeft > 0) {
         if (isBarrelRollingLeft < timeDiff) {
            roll(isBarrelRollingLeft * -2 * M_PI);
         } else {
            roll(timeDiff * -2 * M_PI);
         }
         
         isBarrelRollingLeft -= timeDiff;
      }

      if (isBarrelRollingRight > 0) {
         if (isBarrelRollingRight < timeDiff) {
            roll(isBarrelRollingRight * 2 * M_PI);
         } else {
            roll(timeDiff * 2 * M_PI);
         }

         isBarrelRollingRight -= timeDiff;
      }

}

void AsteroidShip::handleRamShotEffect(double timeDiff) {
   if (isFiring && (currentWeapon == RAM_WEAPON_INDEX || gameState->godMode) && weapons[RAM_WEAPON_INDEX]->isReady()) {
      tracker += 20 * timeDiff;
      flashiness += (float)upOrDown * (float)(rando % 10) * timeDiff * 500;
   }
}

void AsteroidShip::updateZoomLevel(double timeDiff) {
      const float minZoom = 1;
      const float maxZoom = 3;
      zoomFactor = (float) clamp(zoomFactor + (timeDiff * zoomSpeed)
            , minZoom, maxZoom);
}
