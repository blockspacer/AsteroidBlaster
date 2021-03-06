/**
 * BeamShot.cpp
 * Sterling Hirsh
 * Shot type 2
 */

#include "BeamShot.h"
#include "Items/BoundingWall.h"
#include "Utility/SoundEffect.h"

#include <math.h>
#include <algorithm>

materialStruct beamMaterial = {
   {1, 1, 0, .8f},
   {1, 1, 0, .8f},
   {1, 1, 0, .8f},
   {0},
   {1, 1, 0, 1.0f}
};

materialStruct hitBeamMaterial = {
   {1, .5f, 1, .6f},
   {1, .5f, 1, .6f},
   {1, .5f, 1, .6f},
   {0},
   {1, .5f, 1, .6f}
};

materialStruct ballMaterial = {
   {0.3f, 0.3f, 0.3f, 0.8f},
   {0.8f, 0.8f, 0.8f, 0.8f},
   {0, 0,   0.8f, 0.8f},
   {8},
   {0.3f, 0.3f, 0.3f, 0.8f}
};

BeamShot::BeamShot(Point3D& posIn, Vector3D dirIn, int _weaponIndex, AsteroidShip* const ownerIn, const GameState* _gameState) :
   Shot(posIn, dirIn, _weaponIndex, ownerIn, _gameState) {
      type = TYPE_BEAMSHOT;
      lifetime = 1.0;
      // In this context, velocity means direction.
      hitYet = false;
      hitItem = NULL; // We use this to make sure it hits only one thing.
      persist = true;
      lastHitFrame = 0;
      firstFrame = curFrame;
      Point3D endPoint1(*position);
      Point3D endPoint2(*position);
      // Set endPoint2 140 units away.
      length = 140;
      velocity->setLength(length);
      velocity->movePoint(endPoint2);
      velocity->normalize();
      // Correct for position when calculating endpoint1 and 2.
      Vector3D positionVector(*position);
      positionVector = positionVector.scalarMultiply(-1);
      positionVector.movePoint(endPoint1);
      positionVector.movePoint(endPoint2);
      // Now set min/max xyz
      minX = std::min(endPoint1.x, endPoint2.x);
      maxX = std::max(endPoint1.x, endPoint2.x);
      minY = std::min(endPoint1.y, endPoint2.y);
      maxY = std::max(endPoint1.y, endPoint2.y);
      minZ = std::min(endPoint1.z, endPoint2.z);
      maxZ = std::max(endPoint1.z, endPoint2.z);
      
      //shouldConstrain = true;
      /* Make sure beam shots aren't culled from the view frustum (necessary to make them appear)
      */
      shouldBeCulled = false;
      updateBoundingBox();

      forward = new Vector3D(*velocity);
      up = new Vector3D(*ownerIn->up);

      // This is how long the beam is drawn. It is shortened when hit.
      drawLength = length;
      isBeam = true;
      damage = 50 + (owner->weapons[RAILGUN_WEAPON_INDEX]->level*10);

      collisionType = new CollisionRay(length, *velocity, *position);
      
      if (!owner->gameState->godMode) {
         SoundEffect::playSoundEffect("Rail2.wav", owner->position, NULL, owner == owner->gameState->ship, 0.3f);
      }
   }

/**
 * Expire after a certain amount of time.
 */
void BeamShot::update(double timeDiff) {
   if (gameState->gsm != ClientMode) {
      if (!gameState->godMode) {

         if (gameState->getGameTime() - timeFired > lifetime) {
            shouldRemove = true;
         }
      } else {
         if (gameState->getGameTime() - timeFired > 0.1) {
            shouldRemove = true;
         }
      }
   }
}

void BeamShot::drawBeam(bool drawDots) {
   const float distanceDifference = 0.3; // :)
   const float ballOffset = 0.4;
   const float angleDiff = 40; // Degrees per unit of ball helix.
   float beamRadius = 0.2;
   float curTime = gameState->getGameTime();
   float timeLeft;
   const float sphereRadius = 0.05;

   glEnable(GL_LIGHTING);
   glDisable(GL_COLOR_MATERIAL);

   if (!gameState->godMode) {
      timeLeft = lifetime - (curTime - timeFired);
   } else {
      timeLeft = 1.4 - (curTime - timeFired);
   }

   beamRadius *= clamp(timeLeft * timeLeft, 0.1, 1);

   glPushMatrix();
   setMaterial(beamMaterial);
   position->glTranslate();
   glPushMatrix();
   glRotate();
   glScalef(-1.0f, -1.0f, -1.0f);
   // It shrinks, probably should fade out.
   drawCylinder(beamRadius, drawLength);
   
   glPopMatrix();
   if (drawDots) {
      if (!gameState->godMode) {
         // Dots
         Vector3D normal(velocity->getNormalVector());
         setMaterial(ballMaterial);
         for (float distance = 0; distance < drawLength;
               distance += distanceDifference) {
            glPushMatrix();
            velocity->glTranslate(distance);
            glRotated(fmod(curTime, 4) * 90 + (distance * (angleDiff + (timeLeft * timeLeft * 40))), velocity->x,
                  velocity->y, velocity->z);
            normal.glTranslate(ballOffset);
            //glutSolidSphere(0.05 * (1 - timeLeft), 10, 10);
            gluSphere(quadric, sphereRadius, 5,5);
            glPopMatrix();
         }
      }
   }
   glPopMatrix();
}

void BeamShot::draw() {
   //fboBegin();
   if (gameSettings->drawDeferred) {
      GLenum buffers[] = {NOLIGHT_BUFFER, GLOW_BUFFER};
      glDrawBuffers(2, buffers);
   }
   drawBeam(true);
   //fboEnd();
}

void BeamShot::debug() {
   printf("BeamShot::debug(): (min/max/position/direction)\n");
   minPosition->print();
   maxPosition->print();
   position->print();
   velocity->print();
}

/**
 * Don't draw shots in minimap (for now)
 */
void BeamShot::drawInMinimap() {
   return;
}

void BeamShot::hitWall(BoundingWall* wall) {
   shouldConstrain = false;
   return;
}

Point3D BeamShot::getWallIntersectionPoint(BoundingWall* wall) {
   Point3D toReturn = Shot::getWallIntersectionPoint(wall);
   if (hitYet) {
      wall->actuallyHit = false;
   }
   if (wall->actuallyHit) {
      drawLength = position->distanceFrom(toReturn);
   }
   return toReturn;
}
