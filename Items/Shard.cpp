/**
 * Chris Brenton
 * Shard.cpp
 * A randomly generated shard object.
 */

#define _USE_MATH_DEFINES
#include "Utility/GlobalUtility.h"
#include "Graphics/Sprite.h"
#include "Items/Shard.h"
#include "Items/Asteroid3D.h"
#include "Items/AsteroidShip.h"
#include "Shots/Shot.h"
#include "Shots/BeamShot.h"
#include "Shots/TractorBeamShot.h"
#include "Shots/ElectricityShot.h"
#include "Shots/AntiInertiaShot.h"
#include <algorithm>
#include <math.h>
#include "Utility/SoundEffect.h"
#include "Particles/TractorAttractionParticle.h"
#include "Particles/ShardParticle.h"

using namespace std;

/*
 * Basic constructor.
 */
Shard::Shard(double r, double worldSizeIn, const GameState* _gameState) :
   Object3D(_gameState) {
      shouldDrawInMinimap = true;
      worldSize = worldSizeIn;

      transparentZ = -8.0;

      InitShard(r, worldSizeIn);
   }

/*
 * Virtual destructor.
 */
Shard::~Shard() {

}


/*
 * Initialize the shard.
 */
void Shard::InitShard(double r, double worldSizeIn) {
   angle = 0;
   radius = r;
   worldSize = worldSizeIn;

   scalex = scaley = scalez = 1;

   rotationSpeed = 360 * SPINS_PER_SEC; // Degrees per sec.
   axis = new Vector3D(0, 1, 0);
   axis->randomMagnitude();

   velocity = new Vector3D(0, 0, 0);

   minX = minY = minZ = -1;
   maxX = maxY = maxZ = 1;

   sizeX = maxX - minX;
   sizeY = maxY - minY;
   sizeZ = maxZ - minZ;
   collisionRadius = radius;
   updateBoundingBox();

   orbiterOffset = randdouble() * 10;
}

void Shard::drawOtherOrbiters() {
   double curTime = doubleTime();
   const int numBalls = 1; // This is the number of balls per ring, so x3

   // Make it so balls will draw evenly spaced.
   double increment = M_PI * (2.0 / numBalls);
   double t; // parameter for parametric function.
   double dx, dy, dz;
   int j;
   for (int i = 0; i < numBalls; ++i) {
      t = 5 * curTime + increment * i + orbiterOffset;

      for (j = 0; j < 3; ++j) {
         dx = 0;
         dy = 0;
         dz = 0;
         if (j % 3 == 0) {
            dx = sin(t);
            dy = cos(t);
         }
         if (j % 3 == 1) {
            dy = sin(t + M_PI * 4/ 3);
            dz = cos(t + M_PI * 4 / 3);
         }
         if (j % 3 == 2) {
            dz = sin(t + M_PI * 2 / 3);
            dx = cos(t + M_PI * 2 / 3);
         }
         Vector3D rotationOffset(dx, dy, dz);
         rotationOffset.rotateByDegrees(angle, *axis);
         Point3D* particlePoint = new Point3D(*position);
         rotationOffset.movePoint(*particlePoint);
         Vector3D* particleVel = new Vector3D(0, 0, 0);
         ShardParticle::Add(particlePoint, particleVel, gameState);
      }
   }
}

void Shard::drawGlow() {
   // Disable materials.
   glEnable(GL_COLOR_MATERIAL);
   glPushMatrix();
   glTranslated(position->x, position->y, position->z);
   // Push matrix and draw main shard.
   glPushMatrix();
   glRotated(angle, axis->x, axis->y, axis->z);
   glScaled(scalex, scaley, scalez);
   glDisable(GL_LIGHTING);

   glColor4d(0.3, 0.3, 0.7, 1.0);
   drawShard();

   glPopMatrix();

   glDisable(GL_COLOR_MATERIAL);
   glEnable(GL_LIGHTING);
   glPopMatrix();
}

void Shard::drawShard() {
   double w = 0.5;
   double h = 2.0;
   glBegin(GL_TRIANGLE_STRIP);
   for (int i = 0; i < 2; i++) {
      glVertex3d(0.0, h / 2.0, 0.0);
      glVertex3d(-w / 2.0, 0.0, -w / 2.0);
      glVertex3d(0.0, h / 2.0, 0.0);
      glVertex3d(w / 2.0, 0.0, -w / 2.0);
      glVertex3d(0.0, h / 2.0, 0.0);
      glVertex3d(w / 2.0, 0.0, w / 2.0);
      glVertex3d(0.0, h / 2.0, 0.0);
      glVertex3d(-w / 2.0, 0.0, w / 2.0);
      glVertex3d(0.0, h / 2.0, 0.0);
      glVertex3d(-w / 2.0, 0.0, -w / 2.0);
      h *= -1.0;
   }
   glEnd();
}

void Shard::draw() {
   // Disable materials.
   glEnable(GL_COLOR_MATERIAL);
   glColorMaterial(GL_FRONT_AND_BACK, GL_AMBIENT_AND_DIFFUSE);
   glPushMatrix();
   glTranslated(position->x, position->y, position->z);
   // Push matrix and draw main shard.
   glPushMatrix();
   glRotated(angle, axis->x, axis->y, axis->z);
   glScaled(scalex, scaley, scalez);

   //glDisable(GL_LIGHTING);
   //glEnable(GL_CULL_FACE);

   //setMaterial(CrystalMaterial);
   //mesh.draw(false);
   glColor4d(0.8, 0.8, 1.0, 1.0);
   drawShard();

   glPopMatrix();

   glPopMatrix();
   //glEnable(GL_LIGHTING);
   glDisable(GL_COLOR_MATERIAL);
}


/*
 * Update the shard's rotation and position based on elapsed time.
 */
void Shard::update(double timeDiff) {
   Object3D::update(timeDiff);
   angle += rotationSpeed * timeDiff;
   double speed = velocity->getLength();
   if (speed != 0.0) {
      speed *= (1.0 - DECEL_RATE * timeDiff);
      velocity->setLength(speed);
   }
   drawOtherOrbiters();

}

/*
 * Handle a collision with another Object3D.
 */
void Shard::handleCollision(Drawable* other) {
   AsteroidShip* ship;
   Asteroid3D* asteroid;
   Shot* shot;
   Shard* otherShard;
   TractorBeamShot* TBshot; // Not tuberculosis
   if ((ship = dynamic_cast<AsteroidShip*>(other)) != NULL) {
      shouldRemove = true;
      SoundEffect::playSoundEffect("CrystalCollect.wav");
   } else if ((asteroid = dynamic_cast<Asteroid3D*>(other)) != NULL) {
      double speed = asteroid->velocity->getLength();
      velocity->updateMagnitude(*(asteroid->position), *position);
      velocity->setLength(speed);
   } else if ((shot = dynamic_cast<Shot*>(other)) != NULL) {
      double speed = 40;
      if (dynamic_cast<BeamShot*>(other) != NULL) {
         speed = 80; // High speed from hard-hitting railgun.
         velocity->updateMagnitude(*(shot->position), *position);
         velocity->setLength(speed);
      } else if((TBshot = dynamic_cast<TractorBeamShot*>(other)) != NULL) {
         // Pull the shot in.
         const int numParticles = 1;
         // Set the new speed.
         //speed = position->distanceFrom(*TBshot->position) - TBshot->length;
         Vector3D* TBshotToShip = new Vector3D(*position, *TBshot->owner->position);
         TBshotToShip->setLength(1000 / sqrt((1 + TBshotToShip->getLength())));
         addAcceleration(TBshotToShip);

         Vector3D random;
         for (int i = 0; i < numParticles; ++i) {
            Point3D* particlePosition = new Point3D(*position);
            random.randomMagnitude();
            random.movePoint(*particlePosition);
            Vector3D* particleVelocity = new Vector3D(*particlePosition, *TBshot->position);
            particleVelocity->setLength(10); // Speed of the particle.
            particleVelocity->addUpdate(*velocity);
            // Make this go toward the ship.
            TractorAttractionParticle::Add(particlePosition, particleVelocity, TBshot->owner->position, gameState);
         }
      } else if(dynamic_cast<AntiInertiaShot*>(other) != NULL) {
         Vector3D* newVelocity = new Vector3D(*velocity);
         newVelocity->scalarMultiplyUpdate(-0.1);
         addInstantAcceleration(newVelocity);
         if (rotationSpeed > 0) {
            // TODO: Make this time based.
            rotationSpeed = max(rotationSpeed - 0.5, 0.0);
         }
      } else {
         // Set speed to between the speed of the shot and the current speed.
         speed = shot->velocity->getLength();
         speed += velocity->getLength() * 2;
         speed /= 3;
         velocity->updateMagnitude(*(shot->position), *position);
         velocity->setLength(speed);
      }
   } else if ((otherShard = dynamic_cast<Shard*>(other)) != NULL) {
      Vector3D* push = new Vector3D(*other->position, *position);
      if (push->getComparisonLength() == 0) {
         push->randomMagnitude();
      }
      push->setLength(5);
      addAcceleration(push);
   }
}

/*
 * Generate a random radius.
 */
double Shard::randRadius(double r) {
   return (3 * (r / 4)) + ((r / 4) * randdouble());
}

/**
 * Subclasses can extend this, but this draws a sphere on the minimap.
 */
void Shard::drawInMinimap() {
   glPushMatrix();
   position->glTranslate();
   //glDisable(GL_LIGHTING);
   //glColor3f(0.4, 0.5, 0.7);
   setMaterial(WhiteSolid);
   gluSphere(quadric, 1, 4, 2);
   //glEnable(GL_LIGHTING);
   glPopMatrix();
}

void Shard::debug() {
   printf("Shard::debug(): \n");
   minPosition->print();
   maxPosition->print();
   velocity->print();
   printf("--\n");
}
