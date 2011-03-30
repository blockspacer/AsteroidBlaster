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

#ifndef M_PI
#define M_PI           3.14159265358979323846
#endif

#define BOOST_SCALE 2.0

using namespace std;
extern Custodian* custodian;
const double rotationFactor = 3;
static double spin = 90;
static double flashiness = 0;
static double tracker = 0;
static int rando = 1;
static double x2Change = 0.075;
static double y2Change = 0.075;
static double z2Change = 0.3;
static double xChange = 0;
static double yChange = 0;
static double zChange = 0;
static double backChange = 0;

AsteroidShip::AsteroidShip() :
   Object3D(0, 0, 0, 0),     // Initialize superclass
   shotDirection(0, 0, 1) {  // Initialize shot direction to forward
      /* Currently not braking or acceleration. */
      isBraking = false;
      isBoosting = false;
      shakeAmount = 0;
      brakeFactor = 2;
      /* We store acceleration as scalars to multiply forward, right, and up by each tick. */
      curForwardAccel = curRightAccel = curUpAccel = 0;

      yawSpeed = rollSpeed = pitchSpeed = 0;
      maxSpeed = 5; // Units/s, probably will be changed with an upgrade.
      maxBoostSpeed = maxSpeed * 1.5; // Units/s, probably will be changed with an upgrade.

      // Bounding box stuff.
      //shipRadius = 1; NOT USED
      maxX = maxY = maxZ = 0.5;
      minX = minY = minZ = -0.5;
      updateBoundingBox();

      // Orientation vectors.
      upstart = new Vector3D(0, 1, 0);
      upstart->updateMagnitude(0, 1, 0);
      forward->updateMagnitude(0, 0, 1);
      up->updateMagnitude(0, 1, 0);
      right->updateMagnitude(-1, 0, 0);

      // Is the ship firing? Not when it's instantiated.
      isFiring = false;
      shotPhi = shotTheta = 0;
      // The ship's score. This number is displayed to the screen.
      score = 0;
      // The ship's health. This number is displayed to the screen.
      health = 100;
      // The number of shard collected. This number is displayed to the screen.
      nShards = 0;
      // The ship's max motion parameters.
      maxForwardAccel = 10;
      maxRightAccel = 5;
      maxUpAccel = 5;
      maxYawSpeed = maxPitchSpeed = maxRollSpeed = 3;
      //engine upgrade, terminal speed is raised when leved up
      engineUpgrade = 0;


      // TODO: create all of the shooters that this ship will have.
      shooter = new ShootingAI(this);
      flyingAI = new FlyingAI(this);

      // Create our Radar
      radar = new Radar(this);

      // Add weapons to the list!
      weapons.push_back(new Blaster(this));
      weapons.push_back(new RailGun(this));
      /* IF YOU CHANGE THE ORDER OF THIS LIST, CHANGE THE MAGIC NUMBER 2 for the TractorBeam IN THE FUNCTION drawShotDirectionIndicators below.
       */
      weapons.push_back(new TractorBeam(this));
      weapons.push_back(new Electricity(this));
      weapons.push_back(new LawnMower(this));
      weapons.push_back(new Ram(this));
      weapons.push_back(new AntiInertia(this));

      // The ship's currently selected weapon.
      currentWeapon = 0;
      soundHandle = -1;

      cameraOffset = new Vector3D(0, 2, 5);
      currentView = VIEW_THIRDPERSON_SHIP;
   }

/**
 * Set the currently selected weapon to the weapon type specified.
 * The currentWeapon can also be set from nextWeapon() and prevWeapon().
 */
void AsteroidShip::selectWeapon(int weaponType) {
   currentWeapon = weaponType;
}

/**
 * Set the AI to take over firing
 *
 void AsteroidShip::setShootingAI( bool ai )
 {
 }
 */

/**
 * Retrieve the ship's health.
 */
int AsteroidShip::getHealth() {
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
   return nShards;
}

void AsteroidShip::setYawSpeed(double yawAmountIn) {
   yawSpeed = rotationFactor * yawAmountIn;
}

void AsteroidShip::setPitchSpeed(double pitchAmountIn) {
   pitchSpeed = rotationFactor * pitchAmountIn;
}

void AsteroidShip::setRollSpeed(double rollAmountIn) {
   rollSpeed = rotationFactor * rollAmountIn;
}

void AsteroidShip::updatePlayerAcceleration() {
   addAcceleration(new Vector3D(
         forward->scalarMultiply(curForwardAccel).add(
            right->scalarMultiply(curRightAccel).add(
               up->scalarMultiply(curUpAccel)))));
}

void AsteroidShip::setBrake(bool doBrake) {
   isBraking = doBrake;
}

void AsteroidShip::setBoost(bool doBoost) {
   isBoosting = doBoost;
}

/**
 * Set the engine's acceleration.
 */
void AsteroidShip::accelerateForward(int dir) {
   curForwardAccel = dir * maxForwardAccel;
   updatePlayerAcceleration();
}

void AsteroidShip::accelerateUp(int dir) {
   curUpAccel = dir * maxUpAccel;
   updatePlayerAcceleration();
}

void AsteroidShip::accelerateRight(int dir) {
   curRightAccel = dir * maxRightAccel;
   updatePlayerAcceleration();
}

void AsteroidShip::addNewParticle(Point3D& emitter, Vector3D& baseDirection,
      Vector3D& offsetDirectionX, Vector3D& offsetDirectionY, int color) {
   static Vector3D particleVariation;
   static Point3D curPoint;
   static Vector3D initialOffset;
   static Vector3D randomOffset;
   const float randomAmount = 0.1f;
   curPoint = emitter;

   // Translate the point in 2D
   randomOffset.add(offsetDirectionX.scalarMultiply(randomAmount * (randdouble() - 0.5)));
   randomOffset.add(offsetDirectionY.scalarMultiply(randomAmount * (randdouble() - 0.5)));
   randomOffset.add(baseDirection.scalarMultiply(randomAmount * (randdouble() -0.5)));
   randomOffset.scalarMultiplyUpdate(0.01);

   particleVariation.updateMagnitude(baseDirection.scalarMultiply(randdouble() * 2));
   particleVariation.addUpdate(offsetDirectionX.scalarMultiply(randdouble() * 8 - 4));
   particleVariation.addUpdate(offsetDirectionY.scalarMultiply(randdouble() * 8 - 4));
   particleVariation.scalarMultiplyUpdate(0.05);
   //curPoint = position->add(randomPoint);
   initialOffset.movePoint(curPoint);
   randomOffset.movePoint(curPoint);
   EngineParticle::Add(new Point3D(curPoint),
         new Vector3D(baseDirection.add(particleVariation)), color);
}

void AsteroidShip::createEngineParticles(double timeDiff) {
   //add particles in the opposite direction of the acceration

   const float increment = 0.01f;

   //const float length = acceleration->getLength;
   const int newParticlesPerSecond = 30;
   static Vector3D baseParticleAcceleration;
   static Point3D emitter;

   // First do up upAcceleration.
   if (curUpAccel != 0) {
      baseParticleAcceleration = velocity->add(up->scalarMultiply(-curUpAccel * 0.2));
      emitter = *position;
      forward->movePoint(emitter, -0.5);
      for (int i = 0; i <= newParticlesPerSecond * timeDiff; ++i){
         addNewParticle(emitter, baseParticleAcceleration, *forward, *right);
      }
   }

   // Next do right upAcceleration.
   if (curRightAccel != 0) {
      baseParticleAcceleration = velocity->add(right->scalarMultiply(-curRightAccel * 0.2));
      emitter = *position;
      forward->movePoint(emitter, -0.7);
      for (int i = 0; i <= newParticlesPerSecond * timeDiff; ++i){
         addNewParticle(emitter, baseParticleAcceleration, *forward, *up);
      }
   }

   // Next do forward upAcceleration.
   if (curForwardAccel != 0) {
      // We want to do two streams.
      baseParticleAcceleration = velocity->add(forward->scalarMultiply(-curForwardAccel * 0.05));
      Point3D initialPoint(*position);
      forward->movePoint(initialPoint, -0.7 - (curForwardAccel * 0.02));

      // First do the right side.
      right->movePoint(initialPoint, 0.2);
      //baseParticleAcceleration.addUpdate(right->scalarMultiply(0.5));
      for (int i = 0; i <= newParticlesPerSecond * timeDiff; ++i){
         addNewParticle(initialPoint, baseParticleAcceleration, *right, *up, 2);
      }

      // Next do the left side.
      right->movePoint(initialPoint, -0.4);
      //baseParticleAcceleration.addUpdate(right->scalarMultiply(-1));
      for (int i = 0; i <= newParticlesPerSecond * timeDiff; ++i){
         addNewParticle(initialPoint, baseParticleAcceleration, *right, *up, 3);
      }

      /*
      // Next do the middle side.
      right->movePoint(initialPoint, 0.1);
      //baseParticleAcceleration.addUpdate(right->scalarMultiply(0.5));
      for (int i = 0; i <= newParticlesPerSecond * timeDiff; ++i){
         addNewParticle(initialPoint, baseParticleAcceleration, *right, *up, 3);
      }
      */
   }

}

void AsteroidShip::update(double timeDiff) {
   //TODO: Make the shooting and AIs think.

   if (shooter->isEnabled()) {
      shooter->think(timeDiff);
   }
   
   if(flyingAI->isEnabled())
   {
      flyingAI->think(timeDiff);
   } else {
      updatePlayerAcceleration();
   }
   
   if (isBraking) {
      velocity->xMag -= velocity->xMag * timeDiff * brakeFactor;
      velocity->yMag -= velocity->yMag * timeDiff * brakeFactor;
      velocity->zMag -= velocity->zMag * timeDiff * brakeFactor;
   }
   if (isBoosting) {
      velocity->xMag += velocity->xMag * timeDiff * brakeFactor;
      velocity->yMag += velocity->yMag * timeDiff * brakeFactor;
      velocity->zMag += velocity->zMag * timeDiff * brakeFactor;
   }

   double speed = velocity->getLength();
   if (!isBoosting && speed > maxSpeed + (engineUpgrade*2)) {
      velocity->setLength(maxSpeed+ (engineUpgrade*2));
   } else if (isBoosting && speed > (maxBoostSpeed + (engineUpgrade*2))) {
      velocity->setLength(maxBoostSpeed + (engineUpgrade*2));
   }

   Object3D::update(timeDiff);

   roll(timeDiff * rollSpeed);
   pitch(timeDiff * pitchSpeed);
   yaw(timeDiff * yawSpeed);
   
   if (!shooter->isEnabled()) {
      updateShotDirectionVector();
   }

   // Update weapons.
   for (std::vector<Weapon*>::iterator iter = weapons.begin();
         iter != weapons.end(); ++iter) {
      (*iter)->update(timeDiff);
   }

   if (curForwardAccel != 0 || curUpAccel != 0 || curRightAccel != 0) {
      if (soundHandle == -1)
         soundHandle = SoundEffect::playSoundEffect("ShipEngine.wav", true);
   } else {
      if (soundHandle != -1) {
         SoundEffect::stopSoundEffect(soundHandle);
         soundHandle = -1;
      }

   }

   keepFiring();

   if (shakeAmount != 0) {
      shakeAmount -= (float) (5 * shakeAmount * timeDiff);
      if (shakeAmount < 0.01) {
         shakeAmount = 0;
      }
   }

   createEngineParticles(timeDiff);
}

/**
 * This translates an xOffset and yOffset in world coords to
 * a phi and a theta from the ship.
 */
void AsteroidShip::updateShotDirection(double xOffset, double yOffset) {
   aimX = xOffset;
   aimY = yOffset;
   shotPhi = (M_PI/180) * yOffset * VERT_FOV / 2;
   shotTheta = (M_PI/180) * xOffset * VERT_FOV / (/*((double) GW/GH) / */ -2);
   updateShotDirectionVector();
}

void AsteroidShip::fire(bool startFiring) {
   isFiring = startFiring;
}

void AsteroidShip::keepFiring() {
   if (!isFiring) return;
   weapons[currentWeapon]->fire();
}

void draw_ship(double drawOrange){
   glPolygonOffset(1.0f, 1.0f);
   glEnable(GL_POLYGON_OFFSET_FILL);
   glPolygonMode(GL_FRONT_AND_BACK, GL_FILL);

   glDisable(GL_CULL_FACE);
   glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
   glEnable(GL_LIGHTING);
   glEnable(GL_COLOR_MATERIAL);
   glScaled(1.5, .5, .8);
   glScaled(5, 5, 5);

   glBegin(GL_TRIANGLES);
   glEnable(GL_NORMALIZE);

   glNormal3d(.2, -.005, -.03);
   glVertex3d(0, 0, 0);
   glVertex3d(.2, .2, 1.3);
   glVertex3d(.15, 0, 1);

   glNormal3d(-.005, .2, -.03);
   glVertex3d(0, 0, 0);
   glVertex3d(0, .15, 1);
   glVertex3d(.2, .2, 1.3);

   glNormal3d(.005, .2, -.03);
   glVertex3d(0, 0, 0);
   glVertex3d(-.2, .2, 1.3);
   glVertex3d(0, .15, 1);

   glNormal3d(-.2, -.005, -.03);
   glVertex3d(0, 0, 0);
   glVertex3d(-.15, 0, 1);
   glVertex3d(-.2, .2, 1.3);

   glNormal3d(-.2, .005, -.03);
   glVertex3d(0, 0, 0);
   glVertex3d(-.2, -.2, 1.3);
   glVertex3d(-.15, 0, 1);

   glNormal3d(0.005,-0.2,-0.03);
   glVertex3d(0, 0, 0);
   glVertex3d(0, -.15, 1);
   glVertex3d(-.2, -.2, 1.3);

   glNormal3d(-.005,-0.2,-0.03);
   glVertex3d(0, 0, 0);
   glVertex3d(.2, -.2, 1.3);
   glVertex3d(0, -.15, 1);

   glNormal3d(0.2,0.005,-0.03) ;
   glVertex3d(0, 0, 0);
   glVertex3d(.15, 0, 1);
   glVertex3d(.2, -.2, 1.3);
   

   /* Back of Ship */
   glNormal3d(-0.045,-0.045,0.0375);
   glVertex3d(.15, 0, 1);
   glVertex3d(.2, .2, 1.3);
   glVertex3d(0, .15, 1);

   glNormal3d(-0.045,0.045,0.0375);
   glVertex3d(.15, 0, 1);
   glVertex3d(0, -.15, 1);
   glVertex3d(.2, -.2, 1.3);

   glNormal3d(0.045,0.045,0.0375);
   glVertex3d(-.15, 0, 1);
   glVertex3d(-.2, -.2, 1.3);
   glVertex3d(0, -.15, 1);

   glNormal3d(0.045,-0.045,0.0375);
   glVertex3d(-.15, 0, 1);
   glVertex3d(0, .15, 1);
   glVertex3d(-.2, .2, 1.3);
   
   if (drawOrange == 10.0) {
      glColor4d(1, .4, 0, .4);
      backChange += .12/20;
      
      if (backChange > .6) {
         backChange = .6;
      }
      
      //glNormal3d(0.09,0.09,0.0225);
      glVertex3d(.15, 0, 1);
      glVertex3d(0, .15, 1);
      glVertex3d(0, 0, 1 + backChange);

      //glNormal3d(0.09,-0.09,0.0225);
      glVertex3d(.15, 0, 1);
      glVertex3d(0, 0, 1 + backChange);
      glVertex3d(0, -.15, 1);

      //glNormal3d(-0.09,-0.09,0.0225);
      glVertex3d(-.15, 0, 1);
      glVertex3d(0, -.15, 1);
      glVertex3d(0, 0, 1 + backChange);

      //glNormal3d(-0.09,0.09,0.0225);
      glVertex3d(-.15, 0, 1);
      glVertex3d(-0, 0, 1 + backChange);
      glVertex3d(0, .15, 1);
   } else if (backChange == 0) {
      glColor4d(0, 0, 0, .4);

      glVertex3d(-.15, 0, 1);
      glVertex3d(.15, 0, 1);
      glVertex3d(0, .15, 1);
      
      glVertex3d(-.15, 0, 1);
      glVertex3d(.15, 0, 1);
      glVertex3d(0, -.15, 1);
   } else {
      glColor4d(1, .4, 0, .4);
      backChange -= .12/20;
      if (backChange < 0) {
         backChange = 0;
      }
      //glNormal3d(0.09,0.09,0.0225);
      glVertex3d(.15, 0, 1);
      glVertex3d(0, .15, 1);
      glVertex3d(0, 0, 1 + backChange);

      //glNormal3d(0.09,-0.09,0.0225);
      glVertex3d(.15, 0, 1);
      glVertex3d(0, 0, 1 + backChange);
      glVertex3d(0, -.15, 1);

      //glNormal3d(-0.09,-0.09,0.0225);
      glVertex3d(-.15, 0, 1);
      glVertex3d(0, -.15, 1);
      glVertex3d(0, 0, 1 + backChange);

      //glNormal3d(-0.09,0.09,0.0225);
      glVertex3d(-.15, 0, 1);
      glVertex3d(-0, 0, 1 + backChange);
      glVertex3d(0, .15, 1);
   }
   
   glEnd();
   glDisable(GL_POLYGON_OFFSET_LINE);

   /* Outline of Ship */
   glEnable(GL_POLYGON_OFFSET_LINE);
   glPolygonOffset(-1.0f, -1.0f);
   glPolygonMode(GL_FRONT_AND_BACK, GL_LINE);
   
   glLineWidth(1.5);
   glDisable(GL_LIGHTING);
   
   if (drawOrange == 10.0) {
      
      if (xChange > .15) {
         xChange = 0;
         yChange = 0;
         zChange = 0;
      }
      
      if (x2Change > .15) {
         x2Change = 0;
         y2Change = 0;
         z2Change = 0;
      }
      
      if (backChange == .6) {
         glBegin(GL_LINE_LOOP);
         glColor3d(1, .4, 0);
         
         glVertex3d(.15 - xChange, 0, 1 + zChange);
         glVertex3d(0, .15 - yChange, 1 + zChange);
         glVertex3d(-.15 + xChange, 0, 1 + zChange);
         glVertex3d(0, -.15 + yChange, 1 + zChange);
         glVertex3d(.15 - xChange, 0, 1 + zChange);
         
         glEnd();
         
         glBegin(GL_LINE_LOOP);
         glVertex3d(.15 - x2Change, 0, 1 + z2Change);
         glVertex3d(0, .15 - y2Change, 1 + z2Change);
         glVertex3d(-.15 + x2Change, 0, 1 + z2Change);
         glVertex3d(0, -.15 + y2Change, 1 + z2Change);
         glVertex3d(.15 - x2Change, 0, 1 + z2Change);
         glEnd();
         
        

         xChange += .03 / 20;
         yChange += .03 / 20;
         zChange += .12 / 20;
         x2Change += .03 / 20;
         y2Change += .03 / 20;
         z2Change += .12 / 20;
      }
      
      glBegin(GL_LINE_LOOP);
      glColor3d(1, .4, 0);
      
      glVertex3d(.15, 0, 1);
      glVertex3d(.2, .2, 1.3);
      glVertex3d(0, .15, 1);

      glVertex3d(.15, 0, 1);
      glVertex3d(0, 0, 1 + backChange);
      glVertex3d(0, .15, 1);

      glVertex3d(.15, 0, 1);
      glVertex3d(.2, -.2, 1.3);
      glVertex3d(0, -.15, 1);

      glVertex3d(.15, 0, 1);
      glVertex3d(0, 0, 1 + backChange);
      glVertex3d(0, -.15, 1);

      glVertex3d(-.15, 0, 1);
      glVertex3d(-.2, -.2, 1.3);
      glVertex3d(0, -.15, 1);

      glVertex3d(-.15, 0, 1);
      glVertex3d(0, 0, 1 + backChange);
      glVertex3d(0, -.15, 1);

      glVertex3d(-.15, 0, 1);
      glVertex3d(-.2, .2, 1.3);
      glVertex3d(0, .15, 1);

      glVertex3d(-.15, 0, 1);
      glVertex3d(-0, 0, 1 + backChange);
      glVertex3d(0, .15, 1);

      glEnd();
   } else if (backChange == 0) {
      glBegin(GL_LINE_LOOP);
      glColor3d(1, .4, 0);
      glVertex3d(.15, 0, 1);
      glVertex3d(0, .15, 1);
      glVertex3d(-.15, 0, 1);
      glVertex3d(0, -.15, 1);
      glVertex3d(.15, 0, 1);
      glEnd();
   } else {
      glBegin(GL_LINE_LOOP);
      glColor3d(1, .4, 0);
      glVertex3d(.15, 0, 1);
      glVertex3d(0, 0, 1 + backChange);
      glVertex3d(0, .15, 1);
      
      glVertex3d(.15, 0, 1);
      glVertex3d(0, -.15, 1);
      glVertex3d(0, 0, 1 + backChange);
      
      
      glVertex3d(-.15, 0, 1);
      glVertex3d(0, -.15, 1);
      glVertex3d(0, 0, 1 + backChange);
      
      
      glVertex3d(-.15, 0, 1);
      glVertex3d(-0, 0, 1 + backChange);
      glVertex3d(0, .15, 1);
      glEnd();
      
      glBegin(GL_LINE_LOOP);
      glColor3d(1, .4, 0);
      glVertex3d(.15, 0, 1);
      glVertex3d(0, .15, 1);
      glVertex3d(-.15, 0, 1);
      glVertex3d(0, -.15, 1);
      glVertex3d(.15, 0, 1);
      glEnd();
   }

   glBegin(GL_LINES);
   glColor3d(0, 1, 1);
   glVertex3d(0, 0, 0);
   glVertex3d(.2, .2, 1.3);
   
   glVertex3d(0, 0, 0);
   glVertex3d(-.2, .2, 1.3);
   
   glVertex3d(0, 0, 0);
   glVertex3d(-.2, -.2, 1.3);
   
   glVertex3d(0, 0, 0);
   glVertex3d(.2, -.2, 1.3);
   glEnd(); 
   
   glBegin(GL_LINE_LOOP);
   glVertex3d(.15, 0, 1);
   glVertex3d(.2, .2, 1.3);
   glVertex3d(0, .15, 1);
   glVertex3d(-.2, .2, 1.3);
   glVertex3d(-.15, 0, 1);
   glVertex3d(-.2, -.2, 1.3);
   glVertex3d(0, -.15, 1);
   glVertex3d(.2, -.2, 1.3);
   glVertex3d(.15, 0, 1);
   glEnd();

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
   // Counteract the rotation done in GameState::drawInMinimap();
   glRotate();
   const float shipScale = 5;
   glScalef(shipScale, shipScale, shipScale);
   glTranslatef(0, 0, -3);
   glColor4d(0, 0, 0, 0.2);
   //draw_shield();
   draw_ship(curForwardAccel);
   glPopMatrix();
}


void AsteroidShip::draw() {
   glPushMatrix();
   glBlendFunc(GL_ONE, GL_ONE_MINUS_SRC_ALPHA);
   // Translate to the position.
   position->glTranslate();
   // Rotate to the current up/right/forward vectors.
   
   glRotate();
   glTranslated(0, 0, -2.2);
   glColor4d(0, 0, 0, 0.4);

   draw_ship(curForwardAccel);
   glPopMatrix();
}

/**
 * This handles our collisions. Duh.
 * We use a series of dynamic_casts to figure out what we've hit.
 */
void AsteroidShip::handleCollision(Drawable* other) {
   Asteroid3D* asteroid;
   Shard* shard;

   // Try converting other into a Shard
   if ((shard = dynamic_cast<Shard*>(other)) != NULL) {
      health += 3;
      if (health > 100) {
         health = 100;
      }
      nShards++;
      score += 69;
      // Try converting other into an Asteroid3D
   } else if ((asteroid = dynamic_cast<Asteroid3D*>(other)) != NULL) {
      // Decrease the player's health by an appropriate amount.
      addInstantAcceleration(new Vector3D(*(asteroid->velocity)));
      if (!gameState->godMode) {
         health -= (int) (4 * ceil(asteroid->radius));
      }
      if(health < 0) {
         health = 0;
      }
      shakeAmount = 1;
      SoundEffect::playSoundEffect("ShipHit.wav");
   }
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

void AsteroidShip::updateShotDirection(Vector3D dir) {
   shotDirection = dir;
}

Radar *AsteroidShip::getRadar() {
   return radar;
}

void AsteroidShip::updateShotDirection(Point3D dir) {
   shotDirection = Vector3D(dir.x, dir.y, dir.z);
}

/**
 * This is the crosshair for first person.
 */
void AsteroidShip::drawCrosshair() {
   
   if (currentView != VIEW_FIRSTPERSON_SHIP &&
    currentView != VIEW_FIRSTPERSON_GUN) {
      return drawShotDirectionIndicators();
   }

   double crosshairSizeX = 0.05;
   double crosshairSizeY = 0.05;
   double thicknessX = 0.01;
   double thicknessY = 0.01;
   glPushMatrix();
   glLoadIdentity();

   // Make it white
   setMaterial(WhiteSolid);
   glColor3f(1, 1, 1);
   useOrtho();
   glDisable(GL_LIGHTING);
   /*
      glBegin(GL_QUADS);
      glVertex3f(gameState->ship->getAimX() + crosshairSizeX, gameState->ship->getAimY() + thicknessY, 0);
      glVertex3f(gameState->ship->getAimX() - crosshairSizeX, gameState->ship->getAimY() + thicknessY, 0);
      glVertex3f(gameState->ship->getAimX() - crosshairSizeX, gameState->ship->getAimY() - thicknessY, 0);
      glVertex3f(gameState->ship->getAimX() + crosshairSizeX, gameState->ship->getAimY() - thicknessY, 0);

      glVertex3f(gameState->ship->getAimX() + thicknessX, gameState->ship->getAimY() - crosshairSizeY, 0);
      glVertex3f(gameState->ship->getAimX() + thicknessX, gameState->ship->getAimY() + crosshairSizeY, 0);
      glVertex3f(gameState->ship->getAimX() - thicknessX, gameState->ship->getAimY() + crosshairSizeY, 0);
      glVertex3f(gameState->ship->getAimX() - thicknessX, gameState->ship->getAimY() - crosshairSizeY, 0);
      glEnd();
      */
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
   // Don't draw this while firing the tractorbeam.
   // TODO: Make 2 not a magic number.
   if (currentWeapon == 2 && isFiring && weapons[2]->curAmmo > 0 || (gameState->godMode && isFiring)) {
      return;
   }
   // The coords of the boxes.
   Point3D drawPoint = *position;
   const double boxSize = 0.5;

   glPushMatrix();
   shotDirection.movePoint(drawPoint, 5);
   // Start at top right.
   up->movePoint(drawPoint, boxSize / 2);
   right->movePoint(drawPoint, boxSize / 2);
   glDisable(GL_LIGHTING);
   glDisable(GL_CULL_FACE);
   glPolygonMode( GL_FRONT_AND_BACK, GL_LINE );
   setMaterial(WhiteTransparent);

   glLineWidth(3.0);
   glBegin(GL_QUADS);
   // top right
   glColor3d(0.2, 1, 0.0);
   drawPoint.draw();
   up->movePoint(drawPoint, -boxSize);
   drawPoint.draw();
   right->movePoint(drawPoint, -boxSize);
   drawPoint.draw();
   up->movePoint(drawPoint, boxSize);
   drawPoint.draw();

   // Move again
   shotDirection.movePoint(drawPoint, 5);
   // top right
   glColor3d(1, 0.2, 0.0);
   right->movePoint(drawPoint, boxSize);
   drawPoint.draw();
   up->movePoint(drawPoint, -boxSize);
   drawPoint.draw();
   right->movePoint(drawPoint, -boxSize);
   drawPoint.draw();
   up->movePoint(drawPoint, boxSize);
   drawPoint.draw();

   // Move again
   shotDirection.movePoint(drawPoint, 5);
   // top right
   glColor3d(0, 0.2, 1);
   right->movePoint(drawPoint, boxSize);
   drawPoint.draw();
   up->movePoint(drawPoint, -boxSize);
   drawPoint.draw();
   right->movePoint(drawPoint, -boxSize);
   drawPoint.draw();
   up->movePoint(drawPoint, boxSize);
   drawPoint.draw();
   glEnd();
   glPolygonMode( GL_FRONT_AND_BACK, GL_FILL );
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
   return weapons[(currentWeapon + 1) %weapons.size()];
}


/*
 * Just tell us what the previous weapon would be, but don't switch to it.
 */
Weapon* AsteroidShip :: getPreviousWeapon() {
   return weapons[(currentWeapon - 1) %weapons.size()];
}

/**
 * Increment the current weapon and mod by the number of weapons.
 */
void AsteroidShip::nextWeapon() {
   currentWeapon = (currentWeapon + 1) % weapons.size();
}

/**
 * Decrement the current weapon and mod by the number of weapons.
 */
void AsteroidShip::prevWeapon() {
   // We add weaons.size() in the middle so that we don't do -1 % something.
   currentWeapon = (currentWeapon + weapons.size() - 1) % weapons.size();
}

// Return a reference to the list of weapons that the ship has.
std::vector<Weapon*> AsteroidShip :: getWeaponsList() {
   return weapons;
}

// Get the number of types of weapons the ship has. They're indexed 0 - (n-1)
int AsteroidShip :: getNumWeapons() {
   return weapons.size();
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
   if (currentView != VIEW_FIRSTPERSON_SHIP && currentView != VIEW_FIRSTPERSON_GUN) {
      cameraOffset->updateMagnitude(getViewVector()->scalarMultiply(-12));
      cameraOffset->addUpdate(up->scalarMultiply(2));
   } else {
      cameraOffset->updateMagnitude(0, 0, 0);
   }
   return cameraOffset;
}

void AsteroidShip::nextView() {
   setView((currentView + 1) % VIEW_COUNT);
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

void AsteroidShip::setCameraDirectly() {
   gluLookAt(position->x, position->y, position->z,
         position->x + forward->xMag,
         position->y + forward->yMag,
         position->z + forward->zMag,
         up->xMag, up->yMag, up->zMag);
}
