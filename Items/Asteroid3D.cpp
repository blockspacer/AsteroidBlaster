/**
 * Sterling Hirsh
 * Asteroid3D.cpp
 * A randomly generated asteroid object.
 */

#include "Items/Asteroid3D.h"

#include "Utility/GlobalUtility.h"
#include "Utility/SoundEffect.h"

#include "Items/AsteroidShip.h"

#include "Graphics/Texture.h"

#include "Shots/Shot.h"
#include "Shots/BlasterShot.h"
#include "Shots/BeamShot.h"
#include "Shots/TractorBeamShot.h"
#include "Shots/ElectricityShot.h"
#include "Shots/EnergyShot.h"

#include "Graphics/Camera.h"

#include "Particles/ElectricityImpactParticle.h"

#include "Network/gamestate.pb.h"

#include <time.h>
#include <algorithm>
#define _USE_MATH_DEFINES
#include <math.h>

static double timediff = (double) clock();
double randomR = 0;
double randomG = 0;
double randomB = 0;
double energyDamageTime = 5.0;

Asteroid3D::Asteroid3D(double r, double worldSizeIn, const GameState* _gameState, bool isFirst) :
   Object3D(_gameState),
   worldSize(worldSizeIn) {
      type = TYPE_ASTEROID3D;
      shouldDrawInMinimap = true;

      initH = (int)(r * r);
      const float minStartingHealth = 15;
      if (initH < minStartingHealth) {
         initH = minStartingHealth;
      }

      mesh.drawAnim = isFirst;

      lastHitShotOwner = NULL;

      health = initH;

      if (gameState->gsm != ClientMode) {
         newRandomPosition();
      }
      InitAsteroid(r, worldSizeIn);
      newVelocity = new Vector3D();
      newAcceleration = new Vector3D();
      releasedShards = 0;
   }

Asteroid3D::~Asteroid3D() {
   delete newVelocity;
   delete newAcceleration;
}

/**
 * This gives the asteroid a new random position. Useful when
 * initializing them, and not much else.
 */
void Asteroid3D::newRandomPosition() {
   double extent = worldSize - (radius * 2);
   position->x = (randdouble() - 0.5) * extent;
   position->y = (randdouble() - 0.5) * extent;
   position->z = (randdouble() - 0.5) * extent;
   updateBoundingBox();
}

void Asteroid3D::InitAsteroid(double r, double worldSizeIn) {
   angle = 0;
   //radius = r;
   worldSize = worldSizeIn;

   scalex = scaley = scalez = 1;

   rotationSpeed = randdouble() * 100; // Degrees per sec.
   rotationSpeedChange = 0;
   axis = new Vector3D(0, 1, 0);
   axis->randomMagnitude();
   axis->normalize();

   velocity = new Vector3D(0, 0, 0);
   velocity->randomMagnitude();
   velocity->setLength(randdouble() * 3); // Units per sec.

   getBrightColor(randdouble(), cyan, magenta, yellow);

   Ring last;
   double a = randdouble() * 0.25 + 0.875;
   double b = randdouble() * 0.25 + 0.875;

   int npts = (int)((r + 3.0) / 1.5) * 2;

   if (npts == 4) {
      npts += 2;
   }

   for (int j = npts / 2; j >= 0; j--) {
      double tmpangle = (M_PI * 2 / (double)npts * (double)j);
      double tmpRad = r * sin(tmpangle);
      double tmpH = r * cos(tmpangle);
      int tmpPts = (int) (npts * (tmpRad / r));
      if (tmpPts < 0) {
         tmpPts *= -1;
      } else if (tmpPts == 0) {
         tmpPts = 1;
      }

      Ring thisRing;
      thisRing = Ring(tmpPts, a * tmpRad, b * tmpH, &mesh);
      _rList.push_back(thisRing);

      last = thisRing;
   }

   minX = _rList[0].minX();
   maxX = _rList[0].maxX();
   minY = _rList[0].minY();
   maxY = _rList[0].maxY();
   minZ = _rList[0].minZ();
   maxZ = _rList[0].maxZ();

   for (unsigned i = 0; i < _rList.size(); i++) {
      if (_rList[i].minX() < minX) {
         minX = _rList[i].minX();
      }
      if (_rList[i].maxX() > maxX) {
         maxX = _rList[i].maxX();
      }
      if (_rList[i].minY() < minY) {
         minY = _rList[i].minY();
      }
      if (_rList[i].maxY() > maxY) {
         maxY = _rList[i].maxY();
      }
      if (_rList[i].minZ() < minZ) {
         minZ = _rList[i].minZ();
      }
      if (_rList[i].maxZ() > maxZ) {
         maxZ = _rList[i].maxZ();
      }
   }

   for (unsigned i = 1; i <= _rList.size() / 2; i++) {
      makeStrip(_rList[i - 1], _rList[i]);
   }

   for (size_t i = _rList.size() / 2; i < _rList.size() - 1; i++) {
      makeStrip(_rList[i], _rList[i + 1]);
   }

   sizeX = maxX - minX;
   sizeY = maxY - minY;
   sizeZ = maxZ - minZ;

   radius = -minX;
   if (maxX > radius) {
      radius = maxX;
   }
   if (-minY > radius) {
      radius = -minY;
   }
   if (maxY > radius) {
      radius = maxY;
   }
   if (-minZ > radius) {
      radius = -minZ;
   }
   if (maxZ > radius) {
      radius = maxZ;
   }

   // Tweak the radius to (hopefully) encompass all points.
   cullRadius = radius + r * 0.10;

   collisionRadius = radius;
   updateBoundingBox();

   damagePerSecond = 0;
   energyHitAsteroid = false;
   timeLastHitByEnergy = 0;

   collisionType = collisionSphere = new CollisionSphere(radius, *position);
}

void Asteroid3D::draw() {
   glDisable(GL_CULL_FACE);
   setMaterial(Rock);
   
   Object3D::draw(); // This doesn't really do anything.
   glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
   
   glEnable(GL_COLOR_MATERIAL);
   glColorMaterial(GL_FRONT_AND_BACK, GL_AMBIENT_AND_DIFFUSE);
   glPushMatrix();
   position->glTranslate();
   glRotated(angle, axis->x, axis->y, axis->z);
   glScaled(scalex, scaley, scalez);

   // Set the base color to white
   glColor3f(1.0, 1.0, 1.0);
   
   // Draw lines

   glPolygonOffset(1.0f, 1.0f);

   // Draw fill
   glEnable(GL_POLYGON_OFFSET_FILL);
   glPolygonMode(GL_FRONT_AND_BACK, GL_FILL);
   //mesh.draw(false);
   if (gameState->godMode) {
      mesh.drawTextured(true, Texture::getTexture("ZoeRedEyes"));
   } else {
      mesh.drawTextured(true, Texture::getTexture("AsteroidSurface"));
      //mesh.drawTextured(true, 0);
   }
   glDisable(GL_POLYGON_OFFSET_FILL);

   //if (health == 1) {
   //glColor3f(0.996, 0.312, 0.1);
   //} else if (health > 1) {
   double step = (initH - health) / initH;
   double stepR = cyan * step;
   double stepG = magenta * step;
   double stepB = yellow * step;
   //glColor3d(1 - stepR, 1 - stepG, 1 - stepB);
   mesh.setLineColor((float)(1 - stepR),
         (float)(1 - stepG),
         (float)(1 - stepB));
   //}
   //glColor3f(0.996, 0.612, 0.0);

   double shipDist = position->distanceFrom(*gameState->getCurrentCamera()->position);
   double lineW = worldSize / shipDist * ASTEROID3D_LINE_W + 1.0;
   //glLineWidth(ASTEROID3D_LINE_W);
   glLineWidth((GLfloat)lineW);
   glEnable(GL_POLYGON_OFFSET_LINE);
   glPolygonOffset(-1.0f, -1.0f);
   glPolygonMode(GL_FRONT_AND_BACK, GL_LINE);
   //mesh.drawLines(false);
   glDisable(GL_POLYGON_OFFSET_LINE);
   glPolygonMode(GL_FRONT_AND_BACK, GL_FILL);
   glLineWidth(1);
   
   if (energyHitAsteroid) {
      drawEnergyEffect();
   }

   glDisable(GL_COLOR_MATERIAL);
   glPopMatrix();
   glEnable(GL_CULL_FACE);
}

void Asteroid3D::makeStrip(Ring r1, Ring r2) {
   double count = 0.0;
   int last = 0;
   Ring t1, t2;
   if (r1.size() < r2.size()) {
      t1 = r1;
      t2 = r2;
   } else {
      t1 = r2;
      t2 = r1;
   }
   double step = (double)t1.size() / (double)t2.size();
   //for (int i = 0; i < t2.size() - (t2.size() % 2); i++) {
   for (int i = 0; i < t2.size(); i++) {
      int p1, p2, p3, p4;
      p1 = t2._nList[i];
      p2 = t1._nList[(int)count % t1.size()];
      p3 = t2._nList[(i + 1) % t2.size()];
      if (r1.size() < r2.size()) {
         mesh.addFace(p1, p3, p2, gameState);
      } else {
         mesh.addFace(p1, p2, p3, gameState);
      }

      p4 = t1._nList[((int)count - 1 + t1.size()) % t1.size()];
      if (last != (int)count || i == 0) {
      //if (last != (int)count) {
         if (r1.size() < r2.size()) {
            mesh.addFace(p1, p2, p4, gameState);
         } else {
            mesh.addFace(p1, p4, p2, gameState);
         }
      }

      last = (int)count;
      count += step;
   }
}

void Asteroid3D::drawEnergyEffect() {
   double random, posOrNeg;
   double x, y, z;
   double div = 2;
   //double randomR = 0, randomG = 0, randomB = 0;
   
   if (randomR > 1) randomR = .2;
   if (randomG > 1) randomG = .2;
   if (randomB > 1) randomB = .2;
   
   random = (double) (rand() % 4);
   random /= 10;
   randomR += (double) (rand() % 50) / 1000;
   //randomR /= 10;
   randomG += (double) (rand() % 30) / 1000;
   //randomG /= 10;
   randomB += (double) (rand() % 20) / 200;
   //randomB /= 10;
   glPushMatrix();
   glDisable(GL_LIGHTING);

   glColor4d(randomR, randomG, 1, 1);
   glLineWidth(5);

   glRotated(-angle, axis->x, axis->y, axis->z);
   for(double rad = -radius + radius/2; rad < radius; rad += (radius) / 2) {
      glBegin(GL_LINE_LOOP);
      for(double j = 0; j < 2; j += .1) {
            random = (double) (rand() % 4);
            posOrNeg = (double) (rand() % 2);
            posOrNeg = 1 - (posOrNeg * 2);
            random /= 10 * posOrNeg;
            z = ((radius - fabs(rad) + 1 + fabs(rad/div)) * cos(j * M_PI) + random);
            random = (double) (rand() % 4);
            posOrNeg = (double) (rand() % 2);
            posOrNeg = 1 - (posOrNeg * 2);
            random /= 10 * posOrNeg;
            y = ((radius - fabs(rad) + 1 + fabs(rad/div)) * sin(j * M_PI) + random);
            x = rad + random;
            glVertex3d(x, y, z);
      }
      glEnd();
   }
   
   for(double rad = -radius + radius/2; rad < radius; rad += (radius) / 2) {
      glBegin(GL_LINE_LOOP);
      for(double j = 0; j < 2; j += .1) {
            random = (double) (rand() % 4);
            posOrNeg = (double) (rand() % 2);
            posOrNeg = 1 - (posOrNeg * 2);
            random /= 10 * posOrNeg;
            x = ((radius - fabs(rad) + 1 + fabs(rad/div)) * cos(j * M_PI) + random);
            random = (double) (rand() % 4);
            posOrNeg = (double) (rand() % 2);
            posOrNeg = 1 - (posOrNeg * 2);
            random /= 10 * posOrNeg;
            y = ((radius - fabs(rad) + 1 + fabs(rad/div)) * sin(j * M_PI) + random);
            z = rad + random;
            glVertex3d(x, y, z);
      }
      glEnd();
   }
   //for(double rad = -radius; rad <= radius + 1; rad += (radius) / 2) {
   for(double rad = -radius + radius/2; rad < radius; rad += (radius) / 2) {
      glBegin(GL_LINE_LOOP);
      for(double j = 0; j < 2; j += .1) {
            random = (double) (rand() % 4);
            posOrNeg = (double) (rand() % 2);
            posOrNeg = 1 - (posOrNeg * 2);
            random /= 10 * posOrNeg;
            x = ((radius - fabs(rad) + 1 + fabs(rad/div)) * cos(j * M_PI) + random);
            random = (double) (rand() % 4);
            posOrNeg = (double) (rand() % 2);
            posOrNeg = 1 - (posOrNeg * 2);
            random /= 10 * posOrNeg;
            z = ((radius - fabs(rad) + 1 + fabs(rad/div)) * sin(j * M_PI) + random);
            y = rad + random;
            glVertex3d(x, y, z);
      }
      glEnd();
   }
   
   glLineWidth(1);

   glEnable(GL_LIGHTING);
   glPopMatrix();
}

void Asteroid3D::update(double timeDiff) {
   if (gameState->gsm != ClientMode) {
      /* Apply the rotationSpeedChange that we wanted to this asteroid,
       * but never make the rotationSpeed beneath 0.
       * We do this here since Object3D::update() uses rotationSpeed.
       */
      rotationSpeed = std::max(rotationSpeed - rotationSpeedChange * timeDiff, 0.0);
      rotationSpeedChange = 0;
   }
   
   Object3D::update(timeDiff);

   mesh.tick(timeDiff);
   
   if (gameState->gsm != ClientMode) {
      double maxAsteroidSpeed = std::min(gameState->curLevel * 10, 60);

      if (velocity->getComparisonLength() > maxAsteroidSpeed * maxAsteroidSpeed) {
         // Set a speed limit of 60 u/s.
         velocity->setLength(maxAsteroidSpeed);
      }
      
      if (energyHitAsteroid) {
         if (gameState->getGameTime() - timeLastHitByEnergy > 5) {
            energyHitAsteroid = false;
            velocity->updateMagnitude(newVelocity);
            acceleration->updateMagnitude(newAcceleration);         
            damagePerSecond = 0;
         }

         const double rotationSpeedIncreasePerSecond = 20 * damagePerSecond; // deg/sec
         rotationSpeed += timeDiff * rotationSpeedIncreasePerSecond;
         
         health -= timeDiff * damagePerSecond;
      }
      
      if (health <= 0) {
         shouldRemove = true;

         if (radius > 2) {
            int dimension = rand() % 3;
            custodian->add(makeChild(0, dimension));
            custodian->add(makeChild(1, dimension));
         }
         // TODO: Replace this with custodian[lastHitShotOwnerId];
         if (lastHitShotOwner != NULL) {
            lastHitShotOwner->score += (int) radius * 10;
         }

         dropRandomItem();
      }
   }
}

Shard* Asteroid3D::makeShard(int num) {
   Shard* shard;
   shard = new Shard(0.5, worldSize, gameState);
   shard->velocity->updateMagnitude(0.0, 0.0, 0.0);
   shard->position->clone(position);
   shard->position->x += num == 0 ? radius/2 : -radius/2;
   return shard;
}

/**
 * Make a smaller asteroid as the result of an explosion or something.
 * num is the number of asteroid that is produced.
 * This helps with making sure that two asteroids don't intersect.
 */
Asteroid3D* Asteroid3D::makeChild(int num, int dimension) {
   Asteroid3D* asteroid;
   asteroid = new Asteroid3D(radius/2, worldSize, gameState);
   asteroid->velocity->scalarMultiplyUpdate(3);
   asteroid->velocity->addUpdate(*velocity);
   asteroid->position->clone(position);
   if (dimension % 3 == 0)
      asteroid->position->x += num == 0 ? radius/2 : -radius/2;
   else if (dimension % 3 == 1)
      asteroid->position->y += num == 0 ? radius/2 : -radius/2;
   else
      asteroid->position->z += num == 0 ? radius/2 : -radius/2;

   return asteroid;
}

double Asteroid3D::randRadius(double r) {
   return (3 * (r / 4)) + ((r / 4) * randdouble());
}

/**
 * Subclasses can extend this, but this draws a sphere on the minimap.
 */
void Asteroid3D::drawInMinimap() {
   glPushMatrix();
   glColor3f(1, 1, 1);
   glEnable(GL_LIGHTING);
   position->glTranslate();
   setMaterial(RedFlat);
   gluSphere(quadric, 0.5 * collisionRadius, 5, 5);
   glDisable(GL_LIGHTING);
   glPopMatrix();
}

void Asteroid3D::debug() {
   printf("Asteroid3D::debug(): (pos, minPosition, maxPosition, velocity)\n");
   position->print();
   minPosition->print();
   maxPosition->print();
   velocity->print();
   printf("--\n");
}

void Asteroid3D::dropRandomItem() {
   double whichItem = randdouble();
   if (whichItem < 0.6) {
      custodian->add(makeShard(0));
      releasedShards = 1;
   } else if (whichItem < 0.7) {
      releasedShards = 2;
      custodian->add(makeShard(0));
      custodian->add(makeShard(0));
   }
}

bool Asteroid3D::saveDiff(const ast::Entity& old, ast::Entity* ent) {
   bool somethingChanged = Object3D::saveDiff(old, ent);

   if (radius != old.radius()) {
      ent->set_radius(radius);
      somethingChanged = true;
   }

   if (health != old.health()) {
      ent->set_health(health);
      somethingChanged = true;
   }
   
   if (initH != old.healthmax()) {
      ent->set_healthmax(initH);
      somethingChanged = true;
   }
   
   if (energyHitAsteroid != old.energyhit()) {
      ent->set_energyhit(energyHitAsteroid);
      somethingChanged = true;
   }
   
   if (timeLastHitByEnergy != old.timelasthitbyenergy()) {
      ent->set_timelasthitbyenergy(timeLastHitByEnergy);
      somethingChanged = true;
   }
   
   if (damagePerSecond != old.damagepersecond()) {
      ent->set_damagepersecond(damagePerSecond);
      somethingChanged = true;
   }

   if (releasedShards != old.releasedshards()) {
      ent->set_releasedshards(releasedShards);
      somethingChanged = true;
   }
   
   if (rotationSpeed != old.rotationspeed()) {
      ent->set_rotationspeed(rotationSpeed);
      somethingChanged = true;
   }

   return somethingChanged;
}

void Asteroid3D::save(ast::Entity* ent) {
   Object3D::save(ent);
   ent->set_radius(radius);
   ent->set_health(health);
   ent->set_healthmax(initH);
   ent->set_energyhit(energyHitAsteroid);
   ent->set_timelasthitbyenergy(timeLastHitByEnergy);
   ent->set_damagepersecond(damagePerSecond);
   ent->set_releasedshards(releasedShards);
   ent->set_rotationspeed(rotationSpeed);
}

void Asteroid3D::load(const ast::Entity& ent) {
   Object3D::load(ent);
   if (ent.has_radius())
      radius = ent.radius();
   if (ent.has_health())
      health = ent.health();
   if (ent.has_healthmax())
      initH = ent.healthmax();
   if (ent.has_energyhit())
      energyHitAsteroid = ent.energyhit();
   if (ent.has_timelasthitbyenergy())
      timeLastHitByEnergy = ent.timelasthitbyenergy();
   if (ent.has_damagepersecond())
      damagePerSecond = ent.damagepersecond();
   if (ent.has_releasedshards())
      releasedShards = ent.releasedshards();
   if (ent.has_rotationspeed())
      rotationSpeed = ent.rotationspeed();
}

void Asteroid3D::onRemove() {
   if (!enableUI) {
      return;
   }
   // Get the modelview matrix.
   Matrix4 modelView;
   glPushMatrix();
   // Copy the draw() method's transforms.
   glLoadIdentity();
   position->glTranslate();
   glRotated(angle, axis->x, axis->y, axis->z);
   glScaled(scalex, scaley, scalez);
   modelView.loadModelviewMatrix();
   glPopMatrix();

   MeshFace* newFace;
   MeshFace* oldFace;
   for (unsigned i = 0; i < mesh.faces.size(); i++) {
      //mesh.faces[i].position->updateMagnitude();
      // Copy them all.
      oldFace = (mesh.faces[i]);
      newFace = new MeshFace(*oldFace);

      newFace->rotationSpeed = randdouble() * 360;
      newFace->axis = new Vector3D();
      newFace->axis->randomMagnitude();
      newFace->axis->normalize();

      newFace->setTexture(gameState->godMode ? 
       Texture::getTexture("ZoeRedEyes") :
       Texture::getTexture("AsteroidSurface"));
      newFace->timeExploded = gameState->getGameTime();

      // NULL out the pointers that will get copied.
      oldFace->nullPointers();

      // Transform each of the points.
      // We can't use = because p1 - p3 are MeshPoints, not Point3Ds.
      newFace->p1.updateMagnitude(modelView * newFace->p1);
      newFace->p2.updateMagnitude(modelView * newFace->p2);
      newFace->p3.updateMagnitude(modelView * newFace->p3);

      // The new position is the average of the three points.
      newFace->position->updateMagnitude(newFace->p1);
      newFace->position->addUpdate(newFace->p2);
      newFace->position->addUpdate(newFace->p3);

      newFace->position->scalarMultiplyUpdate(1.0 / 3.0);

      // Make each of the points be relative to the position.
      newFace->p1.subtractUpdate(*newFace->position);
      newFace->p2.subtractUpdate(*newFace->position);
      newFace->p3.subtractUpdate(*newFace->position);

      // Add the original asteroid's position.
      newFace->position->addUpdate(*position);

      // Set the new face's velocity.
      newFace->velocity->updateMagnitude(position, newFace->position);
      newFace->velocity->setLength(8);
      newFace->velocity->addUpdate(*velocity);

      // Calculate linear velocity from angular velocity.
      Vector3D positionToFace(*position, *newFace->position);
      Vector3D tmpAxis(*axis); // Normalized.
      double distanceAlongAxis = tmpAxis.dot(positionToFace);
      tmpAxis.setLength(distanceAlongAxis);
      Vector3D spinRadius = positionToFace - tmpAxis;
      Vector3D newDirection = spinRadius.cross(*axis); // This might be backwards.
      newDirection.setLength(-1 * std::min(rotationSpeed * (M_PI / 180.0)
       * spinRadius.getLength(), 30.0));
      //printf("spinRadiusLength: %f, radius: %f\n", spinRadius.getLength(), radius);

      newFace->velocity->addUpdate(newDirection);

      MeshFace::Add(newFace);
   }

   if (health <= 0)
      SoundEffect::playSoundEffect("Explosion1.wav", position, velocity);

   if (releasedShards == 1) {
      SoundEffect::playSoundEffect("CrystalRelease", position);
   }
      //printf("YOU JUST SHARDED\n");
   else if (releasedShards == 2) {
      // Don't play the sound effect twice on top of itself.
      SoundEffect::playSoundEffect("DoubleCrystalRelease", position);
   }
}
