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
#include <algorithm>
#include <math.h>
#include "Utility/SoundEffect.h"
#include "Particles/TractorAttractionParticle.h"
#include "Particles/ShardParticle.h"
#include "Network/gamestate.pb.h"

using namespace std;


/*
 * Basic constructor.
 */
Shard::Shard(double r, double worldSizeIn, const GameState* _gameState) :
   Object3D(_gameState) {
      type = TYPE_SHARD;
      shouldDrawInMinimap = true;
      worldSize = worldSizeIn;

      transparentZ = -8.0;

      InitShard(r, worldSizeIn);

      collisionType = new CollisionSphere(r, *position);
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
   if (gameState->gsm == SingleMode || gameState->gsm == ClientMode) {
      shardType = SHARD_TYPE_MONEY;
   } else {
      double randNum = randdouble();
      double weaponShardCutoff = 0;
      // Make weapon shards spawn less as the levels progress.
      if (gameState->curLevel > 4) {
         weaponShardCutoff = 0;
      } else {
         weaponShardCutoff = 0.25 - (0.05 * gameState->curLevel);
      }

      if (randNum < 0.02) {
         shardType = SHARD_TYPE_LIFE;
      } else if (randNum < 0.1) {
         shardType = SHARD_TYPE_MAXHEALTH;
      } else if (randNum < 0.2) {
         shardType = SHARD_TYPE_REGEN;
      } else if (randNum < 0.3) {
         shardType = SHARD_TYPE_ENGINE;
      } else if (randNum < 0.5 + weaponShardCutoff) {
         shardType = SHARD_TYPE_WEAPON;
      } else {
         shardType = SHARD_TYPE_HEALTH;
      }
   }
   weapNum = NUMBER_OF_WEAPONS;
}

void Shard::drawOtherOrbiters() {
   double curTime = gameState->getGameTime();
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

void Shard::drawShard() {
   double w = 0.5;
   double h = 2.0;
   switch (shardType) {
      case SHARD_TYPE_HEALTH:
         glColor3d(0.3, 0.3, 0.9);
         break;
      
      case SHARD_TYPE_WEAPON:
         glColor3d(0.9, 0.0, 0.0);
         break;

      case SHARD_TYPE_REGEN:
         glColor3d(0.0, 0.0, 0.9);
         break;

      case SHARD_TYPE_ENGINE:
         glColor3d(1.0, 1.0, 0.0);
         break;

      case SHARD_TYPE_MAXHEALTH:
         glColor3d(0.0, 0.9, 0.9);
         break;

      case SHARD_TYPE_LIFE:
         glColor3d(0.0, 0.9, 0.0);
         break;

      case SHARD_TYPE_MONEY:
      default:
         glColor3d(0.8, 0.8, 1.0);
   }
   //fboBegin();
   if (gameSettings->drawDeferred) {
      GLenum buffers[] = {NORMAL_BUFFER, GL_NONE, ALBEDO_BUFFER, GLOW_BUFFER};
      glDrawBuffers(4, buffers);
      // Used to be bonerShader, but I don't know why.
      glUseProgram(gBufferShader);
   }
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
   if (gameSettings->drawDeferred) {
      glUseProgram(0);
   }
   //fboEnd();
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
   //glColor4d(0.8, 0.8, 1.0, 1.0);
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
   if (gameState->gsm != ClientMode) {
      double speed = velocity->getLength();
      if (speed != 0.0) {
         speed *= (1.0 - DECEL_RATE * timeDiff);
         velocity->setLength(speed);
      }
   }

   drawOtherOrbiters();
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
   glEnable(GL_LIGHTING);
   //glColor3f(0.4, 0.5, 0.7);
   setMaterial(WhiteSolid);
   gluSphere(quadric, 1, 4, 2);
   //glEnable(GL_LIGHTING);
   glDisable(GL_LIGHTING);
   glPopMatrix();
}

void Shard::debug() {
   printf("Shard::debug(): \n");
   minPosition->print();
   maxPosition->print();
   velocity->print();
   printf("--\n");
}

void Shard::save(ast::Entity* ent) {
   Object3D::save(ent);
   ent->set_shardtype(shardType);
   ent->set_weapnum(weapNum);
}

bool Shard::saveDiff(const ast::Entity& old, ast::Entity* ent) {
   // This is here for easy extensibility.
   bool changed = Object3D::saveDiff(old, ent);

   if (shardType != old.shardtype()) {
      changed = true;
      ent->set_shardtype(shardType);
   }

   if (weapNum != old.weapnum()) {
      changed = true;
      ent->set_weapnum(weapNum);
   }

   return changed;
}

void Shard::load(const ast::Entity& ent) {
   Object3D::load(ent);
   if (ent.has_shardtype()) {
      shardType = ent.shardtype();
   }
   if (ent.has_weapnum()) {
      weapNum = ent.weapnum();
   }
}
