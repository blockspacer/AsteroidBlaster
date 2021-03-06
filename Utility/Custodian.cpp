/**
 * Custodian - Stores where all the stuff is.
 * @author Sterling Hirsh
 * @date 1-26-11
 * CPE 476 
 */

#include "Utility/Custodian.h"
#include "Utility/Collision.h"

#include "Utility/SoundEffect.h"

#include <sstream>
#include <algorithm>
#include <assert.h>
#include <vector>

#include "Items/Asteroid3D.h"
#include "Items/Shard.h"
#include "Shots/BeamShot.h"
#include "Shots/BlasterShot.h"
#include "Shots/TractorBeamShot.h"
#include "Shots/ElectricityShot.h"
#include "Shots/EnergyShot.h"
#include "Shots/ExplosiveShot.h"
#include "Shots/TimedBombShot.h"
#include "Shots/HomingMissileShot.h"

#include "Particles/ElectricityImpactParticle.h"
#include "Particles/BlasterImpactParticle.h"
#include "Particles/TractorAttractionParticle.h"

#include "Text/GameMessage.h"

#include "Network/gamestate.pb.h"

const unsigned defaultNextID = 1;

/**
 * AsteroidShip collisions======================================================
 */
template<>
void Collision<AsteroidShip, AsteroidShip>::handleCollision() {
   if (a->gameState->gsm == MenuMode || a->gameState->gsm == ClientMode)
      return;

   if (!(a->isVulnerable())) { return;}
   if (!(b->isVulnerable())) { return;}

   double d = a->position->distanceFrom(*b->position);
   double combinedRad = a->radius + b->radius;
   if (combinedRad < d) {
      return;
   }
   
   double maxR = std::max(a->radius, b->radius);
   Vector3D* pushOnA = new Vector3D(*b->position, *a->position);
   Vector3D* pushOnB = new Vector3D(pushOnA->scalarMultiply(-1));

   if (d <= maxR) {
      //printf("d <= maxR, d=%f|maxR=%f\n",d,maxR);
      if (d == 0) {
         //printf("AsteroidShip is stuck\n");
         pushOnA->randomMagnitude();
      }
      pushOnA->setLength(1.0); // Units per sec per sec.

      *pushOnB = pushOnA->scalarMultiply(-1);

      a->addAcceleration(pushOnA);
      b->addAcceleration(pushOnB);
   }

   Vector3D reflectionAxis(*b->position, *a->position);
   reflectionAxis.normalize();

   double m1 = a->radius; // This Ship's Mass.
   double m2 = b->radius; // Other Ship's Mass.

   Vector3D* newVelocity_a = new Vector3D(*a->velocity);
   Vector3D* newVelocity_b = new Vector3D(*b->velocity);

   // Added the 0.2 to make stuff lose some energy.
   newVelocity_a->scalarMultiplyUpdate(m1 - m2);
   newVelocity_a->addUpdate(b->velocity->scalarMultiply(2 * m2));
   newVelocity_a->scalarMultiplyUpdate(1 / (m1 + m2));
   
   newVelocity_b->scalarMultiplyUpdate(m2 - m1);
   newVelocity_b->addUpdate(a->velocity->scalarMultiply(2 * m1));
   newVelocity_b->scalarMultiplyUpdate(1 / (m1 + m2));
   
   // We want to only add acceleration away from the other item.
   if (newVelocity_a->dot(*pushOnA) < 0) {
      newVelocity_a->scalarMultiplyUpdate(-1);
   }
   
   if (newVelocity_b->dot(*pushOnB) < 0) {
      newVelocity_b->scalarMultiplyUpdate(-1);
   }
   
   a->addInstantAcceleration(newVelocity_a);
   b->addInstantAcceleration(newVelocity_b);

   Vector3D* reverseVelocity = new Vector3D(*a->velocity);
   reverseVelocity->scalarMultiplyUpdate(-1);
   a->addInstantAcceleration(reverseVelocity);

   reverseVelocity = new Vector3D(*b->velocity);
   reverseVelocity->scalarMultiplyUpdate(-1);
   b->addInstantAcceleration(reverseVelocity);
}

template<>
void Collision<AsteroidShip, Asteroid3D>::handleCollision() {
   if (a->lives <= 0) { return; }
   if (a->isRespawning()) { return; }
   if (a->isVulnerable()) {
      /* Remove health from both equal to minHealthDeduction + randomHealthDeduction 
       * or b->health or a->health, whichever is smallest.
       */
      const float minHealthDeduction = 50;
      const float randomHealthDeduction = randdouble() * 40;

      if (a->gameState->gsm != ClientMode) {
         double healthToRemove = std::min(std::min(a->health, b->health), 
          minHealthDeduction + randomHealthDeduction);
         
         a->health -= healthToRemove;
         b->health -= healthToRemove;
         
         a->addInstantAcceleration(new Vector3D(*(b->velocity)));
         a->addInstantAcceleration(new Vector3D(*b->position, *a->position));
         b->addInstantAcceleration(new Vector3D(*(a->velocity)));
         b->addInstantAcceleration(new Vector3D(*a->position, *b->position));
         
         a->lastDamagerId = b->id;
         a->lastDamagerWeapon = DAMAGER_INDEX_ASTEROID;
      }

      a->shakeAmount = 8;

      Vector3D shipToAsteroid(a->position, b->position);
      shipToAsteroid.normalize();
      Point3D collisionPoint(a->position);
      collisionPoint.addUpdate(shipToAsteroid.scalarMultiply(a->radius));

      SoundEffect::playSoundEffect("ShipHit.wav", &collisionPoint);

      a->justGotHit = a->gameState->getGameTime();
   } else {
      if (a->gameState->gsm != ClientMode) {
         const float minHealthDeduction = 50;
         const float randomHealthDeduction = randdouble() * 40;
      
         float healthToRemove = std::min(std::min(a->health, b->health), 
          minHealthDeduction + randomHealthDeduction);

         b->health -= healthToRemove;
      }
   }
}

template<>
void Collision<AsteroidShip, Shard>::handleCollision() {
   if (a->isRespawning()) { return;}

   switch (b->shardType) {
      case SHARD_TYPE_HEALTH:
         if (a->gameState->gsm != ClientMode) {
            a->health += 10;
            if (a->health > a->healthMax)
               a->health = a->healthMax;
         }
         break;
      case SHARD_TYPE_WEAPON:
         {
            // TODO: Messages from weapons.
            // Pick a random weapon. If it's purchased, levelup. Else, get it.
            int startWeapNum = rand() % NUMBER_OF_WEAPONS;
            int weapNum = (startWeapNum + 1) % NUMBER_OF_WEAPONS;

            bool pickWeapNum = ((int)b->weapNum == NUMBER_OF_WEAPONS);

            Weapon* weapon;
            if (a->gameState->gsm != ClientMode) {
               if (pickWeapNum) {
                  while (weapNum != startWeapNum) {
                     weapon = a->getWeapon(weapNum);
                     if (weapon == NULL) {
                        std::cerr << "Null weapon " << weapNum << " in Ship/Shard collision part 1." << std::endl;;
                        break;
                     }
                     
                     if (!weapon->purchased) {
                        weapon->purchased = true;
                        break;
                     } else if (weapon->level < weapon->levelMax) {
                        weapon->level++;
                        break;
                     }
                     weapNum++;
                  }
                  // Send it to the client.
                  b->weapNum = weapNum;
               } else {
                  weapNum = b->weapNum;
                  weapon = a->getWeapon(weapNum);
                  if (weapon == NULL) {
                     std::cerr << "Null weapon " << weapNum << " in Ship/Shard collision part 2." << std::endl;;
                  } else if (!weapon->purchased) {
                     weapon->purchased = true;
                  } else if (weapon->level < weapon->levelMax) {
                     weapon->level++;
                  }
                  // Weap num is already set.
               }
            } else {
               weapNum = b->weapNum;
            }

            if (a == a->gameState->ship) {
               weapon = a->getWeapon(weapNum);
               if (weapon == NULL) {
                  std::cerr << "Null weapon " << weapNum << " in Ship/Shard collision part 3." << std::endl;;
               }
               assert(weapon != NULL);
               
               if (!weapon->purchased) {
                  a->gameState->addWeaponUnlockMessage(weapon);
               } else if (weapon->level < weapon->levelMax) {
                  a->gameState->addWeaponUnlockMessage(weapon);
               }
            }
            // If we get here, oh well.
         }
         break;

      case SHARD_TYPE_REGEN:
         if (a->gameState->gsm != ClientMode) {
            if (a->regenHealthLevel + 1 < a->regenHealthLevelMax()) {
               a->regenHealthLevel += 1;

               if (a == a->gameState->ship) {
                  std::ostringstream gameMsg;
                  gameMsg << "Unlocked Regen Health level " << a->regenHealthLevel;
                  GameMessage::Add(gameMsg.str(), 30, 3, a->gameState);
               }
            }
         } else {
            if (a == a->gameState->ship) {
               std::ostringstream gameMsg;
               gameMsg << "Unlocked Regen Health level " << a->regenHealthLevel;
               GameMessage::Add(gameMsg.str(), 30, 3, a->gameState);
            }
         }
         break;

      case SHARD_TYPE_ENGINE:
         if (a->gameState->gsm != ClientMode) {
            a->engineLevel += 1;
         }
         if (a == a->gameState->ship) {
            std::ostringstream gameMsg;
            gameMsg << "Unlocked Engine level " << a->engineLevel;
            GameMessage::Add(gameMsg.str(), 30, 3, a->gameState);
         }
         break;

      case SHARD_TYPE_MAXHEALTH:
         if (a->gameState->gsm != ClientMode) {
            a->healthMax += 10;
            a->health = a->healthMax;
         }
            
         if (a == a->gameState->ship) {
            std::ostringstream gameMsg;
            gameMsg << "Shields upgraded to " << a->healthMax << "%";
            GameMessage::Add(gameMsg.str(), 30, 3, a->gameState);
         }
         break;

      case SHARD_TYPE_LIFE:
         if (a->gameState->gsm != ClientMode) {
            a->lives++;
         }

         if (a == a->gameState->ship) {
            std::ostringstream gameMsg;
            gameMsg << "Extra Life!";
            GameMessage::Add(gameMsg.str(), 30, 3, a->gameState);
         }
         break;
      
      case SHARD_TYPE_MONEY:
      default:
         if (a->gameState->gsm != ClientMode) {
            a->health += 10;
            if (a->health > a->healthMax)
               a->health = a->healthMax;

            a->unbankedShards++;
            a->score += 69;
         }
   }

   if (a->gameState->gsm != ClientMode) {
      b->shouldRemove = true;
   }
   
   if (a == a->gameState->ship) {
      SoundEffect::playSoundEffect("CrystalCollect.wav", NULL, NULL, true);
   } else {
      SoundEffect::playSoundEffect("CrystalCollect.wav", a->position);
   }
}

template<>
void Collision<AsteroidShip, BlasterShot>::handleCollision() {
   if (!(a->isVulnerable())) { return;}
   if (a->isRespawning()) { return;}
   int particlesToEmit = 30;
   if (a != b->owner) {
      if (a->gameState->gsm != ClientMode) {
         a->health -= b->getDamage(a);
         b->shouldRemove = true;
         a->lastDamagerId = b->owner->id;
         a->lastDamagerWeapon = b->weaponIndex;
      }

      a->shakeAmount = 0.5f;
      a->justGotHit = a->gameState->getGameTime();
      SoundEffect::playSoundEffect("BlasterHit.wav", b->position);

      for (int i = 0; i <= particlesToEmit; ++i) {
         Point3D* particleStartPoint = new Point3D(*(b->position));
         Vector3D* particleDirection = new Vector3D();
         particleDirection->randomMagnitude();
         particleDirection->setLength(3);
         BlasterImpactParticle::Add(particleStartPoint, particleDirection, b->gameState);
      }
   }
}

template<>
void Collision<AsteroidShip, BeamShot>::handleCollision() {
   if (!(a->isVulnerable())) { return;}
   if (a->isRespawning()) { return;}
   if (a != b->owner && !b->hitYet && curFrame - 1 <= b->firstFrame) {
      //TODO addInstantVelocity b->velocity->scalarMultiply(10)
      if (a->gameState->gsm != ClientMode) {
         a->health -= b->getDamage(a);
         b->owner->score += (int) a->radius * 10;
         a->addInstantAcceleration(new Vector3D(b->velocity->scalarMultiply(20)));
         a->lastDamagerId = b->owner->id;
         a->lastDamagerWeapon = b->weaponIndex;
      }
      a->shakeAmount = 0.75f;
      b->hitYet = true;
      b->hitItem = a;
      b->lastHitFrame = curFrame;
      b->drawLength = a->position->distanceFrom(*b->position);
      a->justGotHit = a->gameState->getGameTime();

      SoundEffect::playSoundEffect("BlasterHit.wav", b->position);
   }
}

template<>
void Collision<AsteroidShip, ElectricityShot>::handleCollision() {
   if (!(a->isVulnerable())) { return;}
   if (a->isRespawning()) { return;}
   if (b->damage == 0) { return;}
   const int numElecParticles = 1;
   double hitDistance = 0;

   if (a != b->owner) {

      /* Normal collision detection gets done in CollisionTypes.h. 
       * I can't think of an efficient way to do this, so I'm leaving it here
       * to avoid creating a hack.
       */
      Point3D* closestPoint = a->sphereCollideWithRay(*b->position, *b->velocity, &hitDistance);

      // Sometimes, we decide that it's not a real hit. No big deal.
      if (closestPoint != NULL) {
         b->hitYet = true;
         if (a->gameState->gsm != ClientMode) {
            a->health -= b->getDamage(a);
            a->lastDamagerId = b->owner->id;
            a->lastDamagerWeapon = b->weaponIndex;
         }
         a->shakeAmount = 0.7f;
         a->justGotHit = a->gameState->getGameTime();

         b->length = hitDistance;
         Vector3D centerToImpactPoint(*a->position, *closestPoint);
         centerToImpactPoint.setLength(5);
         for (int i = 0; i < numElecParticles; ++i) {
            Point3D* particleStartPoint = new Point3D(*closestPoint);
            Vector3D* particleDirection = new Vector3D();
            particleDirection->randomMagnitude();
            particleDirection->setLength(3);
            particleDirection->addUpdate(centerToImpactPoint);
            ElectricityImpactParticle::Add(particleStartPoint, particleDirection, b->gameState);
         }
         SoundEffect::playSoundEffect("BlasterHit.wav", closestPoint);

         delete closestPoint;
      }
   }
}

template<>
void Collision<AsteroidShip, EnergyShot>::handleCollision() {
   if (!(a->isVulnerable())) { return;}
   if (a->isRespawning()) { return;}
   if (a != b->owner) {
      if (a->gameState->gsm != ClientMode) {
         a->health -= clamp(b->chargeTime, 0, 5) * 15.0;
         a->velocity->updateMagnitude(0, 0, 0);
         a->acceleration->updateMagnitude(0, 0, 0);
         b->shouldRemove = true;
         a->lastDamagerId = b->owner->id;
         a->lastDamagerWeapon = b->weaponIndex;
      }


      SoundEffect::playSoundEffect("BlasterHit.wav", b->position);
      if (b->weapon->chargingShotid == b->id) {
         b->weapon->resetChargingShot();
      }
      a->shakeAmount = 0.7f;
      a->justGotHit = a->gameState->getGameTime();
   }
}

template<>
void Collision<AsteroidShip, HomingMissileShot>::handleCollision() {
   if(a == b->owner || !a->isVulnerable()) 
      return;

   if (!b->isExploded) {
      Vector3D positionToAsteroid(*b->position, *a->position);
      double distance = positionToAsteroid.getLength();
      if (distance < b->seekRadius + a->radius) {
         if (distance > b->collisionRadius + a->radius) {
            if (a->gameState->gsm != ClientMode) {
               Vector3D toAsteroidNorm(positionToAsteroid);
               toAsteroidNorm.normalize();         
               double forwardAmount = b->forward->dot(toAsteroidNorm);

               if (forwardAmount > .75) {
                  if (b->targetID == -1) {
                     b->targetID = a->id;
                  } else {
                     Object3D* target = a->gameState->custodian[b->targetID];
                     if(target == NULL) {
                        b->targetID = a->id;
                     } else {
                        Vector3D positionToExistingAsteroid(*b->position, *(target->position));
                        Vector3D toExistingAsteroidNorm(positionToExistingAsteroid);
                        toExistingAsteroidNorm.normalize();

                        double existingForwardAmount = b->forward->dot(toExistingAsteroidNorm);
                        if (forwardAmount > .95 && existingForwardAmount > .95) {
                           double otherDistance = positionToExistingAsteroid.getLength();
                           if (distance < otherDistance) {
                              b->targetID = a->id;
                           }
                        } else if (forwardAmount > existingForwardAmount) {
                           b->targetID = a->id;
                        }
                     }
                  }
               }
            }
         } else {
            b->shouldExplode = true;
            if (a->gameState->gsm != ClientMode) {
               a->health -= b->getDamage(a);
               a->justGotHit = a->gameState->getGameTime();
               a->lastDamagerId = b->owner->id;
               a->lastDamagerWeapon = b->weaponIndex;
            }
            b->hasDamaged = true;
            SoundEffect::playSoundEffect("BlasterHit.wav", b->position);
         }
      }
   } else {
      // Seek
      Vector3D positionToAsteroid(*b->position, *a->position);
      double distance = positionToAsteroid.getLength();
      if (a->gameState->gsm != ClientMode &&
       !(b->hasDamaged) && distance < b->collisionRadius + a->radius) {
         Vector3D* shotToAsteroid;
         shotToAsteroid = new Vector3D(*b->position, *a->position);
         double distance = shotToAsteroid->getLength() - a->radius;
         //printf("Asteroid's health is: %f\n", a->health);
         double newSpeed = 1 / ((1 + (distance * distance) / b->explodeRadius + 1) * a->radius);
         shotToAsteroid->setLength(newSpeed);
         a->addInstantAcceleration(shotToAsteroid);
      }
   }
}

template<>
void Collision<AsteroidShip, TimedBombShot>::handleCollision() {
   if (!(a->isVulnerable())) { return;}

   if (a->isRespawning() || a == b->owner) { return;}
   if (!b->isExploded) {
      Vector3D positionToShip(*b->position, *a->position);
      double distance = positionToShip.getLength();
      if (distance < b->seekRadius + a->radius) {
         if (distance > b->collisionRadius + a->radius) {
            //Vector3D* attraction = new Vector3D(positionToShip);
            //attraction->setLength(20.0);
            //b->addAcceleration(attraction);
         } else {
            b->shouldExplode = true;
            SoundEffect::playSoundEffect("BlasterHit.wav", b->position);
         }
      }
   } else {
      if (a->gameState->gsm != ClientMode) {
         a->health -= b->getDamage(a);
         Vector3D* shotToShip = new Vector3D(*b->position, *a->position);
         double distance = shotToShip->getLength() - a->radius;
         double newSpeed = 1000 / ((1 + (distance * distance) / b->explodeRadius + 1) * a->radius);
         shotToShip->setLength(newSpeed);
         a->addInstantAcceleration(shotToShip);
         a->lastDamagerId = b->owner->id;
         a->lastDamagerWeapon = b->weaponIndex;
      }
      a->shakeAmount = 2;
      a->justGotHit = a->gameState->getGameTime();
   }
}

/**
 * Asteroid3D collisions========================================================
 */
template<>
void Collision<Asteroid3D, BlasterShot>::handleCollision() {
   const int particlesToEmit = 10;
   
   Vector3D particleVariation;
   Vector3D positionDifference;
   const double particleSpeed = 15;
   double speed = 6 / (a->radius);

   if (a->gameState->gsm != ClientMode) {
      b->shouldRemove = true;
      a->health -= b->getDamage(a);
      
      Vector3D* newAcceleration = new Vector3D(*b->position, *a->position);
      newAcceleration->setLength(speed);
      a->addInstantAcceleration(newAcceleration);
      a->lastHitShotOwner = b->owner;
   }
   
   SoundEffect::playSoundEffect("BlasterHit.wav", b->position);
   // Make some particles!

   positionDifference.updateMagnitude(*a->position, *b->position);
   positionDifference.setLength(particleSpeed);

   for (int i = 0; i <= particlesToEmit; ++i) {
      particleVariation.randomMagnitude();
      particleVariation.setLength(particleSpeed);
      particleVariation.addUpdate(positionDifference);
      BlasterImpactParticle::Add(new Point3D(*b->position), 
       new Vector3D(particleVariation), b->gameState);
   }
}

template<>
void Collision<Asteroid3D, BeamShot>::handleCollision() {
   if (!b->hitYet && curFrame - 1 <= b->firstFrame) {
      if (a->gameState->gsm != ClientMode) {
         a->velocity->addUpdate(b->velocity->scalarMultiply(10));
         a->health -= b->getDamage(a);
         b->owner->score += (int) a->radius * 10;
         a->lastHitShotOwner = b->owner;
      }

      b->hitYet = true;
      b->hitItem = a;
      b->lastHitFrame = curFrame;
      b->drawLength = a->position->distanceFrom(*b->position);
   }
}

template<>
void Collision<Asteroid3D, TractorBeamShot>::handleCollision() {
   if (a->gameState->gsm != ClientMode) {
      // Slowly match the Asteroid's speed to the ship's speed.
      // Weight the new velocity towards the ship's velocity.
      Vector3D* tmp = new Vector3D(*a->velocity + (*b->owner->velocity)*2);
      // Divide by 3 since there's 3 vectors added together.
      *tmp /= 3;
      // This is deleted in Object3D::updateAcceleration
      Vector3D* newOne = new Vector3D(*tmp - *a->velocity);
      a->addAcceleration(newOne);
      
      // Add a rotationSpeedChange which the Asteroid3D will apply to itself
      // next frame's update() in a time based manner.
      a->rotationSpeedChange += 15;
   }
}

template<>
void Collision<Asteroid3D, ElectricityShot>::handleCollision() {
   if (b->damage == 0) { return;}
   const int numElecParticles = 1;
   double hitDistance = 0;

   if (a->gameState->gsm != ClientMode) {
      a->health -= b->getDamage(a);
      a->lastHitShotOwner = b->owner;
   }
   
   //TODO: Add a bit of push.
   
   /* Normal collision detection gets done in CollisionTypes.h. 
    * I can't think of an efficient way to do this, so I'm leaving it here
    * to avoid creating a hack.
    */
   Point3D* closestPoint = a->sphereCollideWithRay(*b->position, *b->velocity, &hitDistance);

   // Sometimes, we decide that it's not a real hit. No big deal.
   if (closestPoint != NULL) {
      b->length = hitDistance;
      Vector3D centerToImpactPoint(*a->position, *closestPoint);
      centerToImpactPoint.setLength(5);
      for (int i = 0; i < numElecParticles; ++i) {
         Point3D* particleStartPoint = new Point3D(*closestPoint);
         Vector3D* particleDirection = new Vector3D();
         particleDirection->randomMagnitude();
         particleDirection->setLength(3);
         particleDirection->addUpdate(centerToImpactPoint);
         ElectricityImpactParticle::Add(particleStartPoint, particleDirection, b->gameState);
      }

      SoundEffect::playSoundEffect("BlasterHit.wav", closestPoint);
      delete closestPoint;
   }
}

template<>
void Collision<Asteroid3D, EnergyShot>::handleCollision() {
   // Calculate any remaining damage to be done, and factor it in to the new damage per second.
   // TODO: 5.0 is the time damage is dealt. We may want to put this in energyshot.

   const double energyDamageTime = 5.0;


   if (a->gameState->gsm != ClientMode) {
      double damageTimeLeft = clamp(a->gameState->getGameTime() - a->timeLastHitByEnergy, 0.0, energyDamageTime);
      double damageLeft = a->damagePerSecond * damageTimeLeft;

      a->lastHitShotOwner = b->owner;
      a->damagePerSecond = (damageLeft / energyDamageTime) + b->damagePerSecond;
      a->timeLastHitByEnergy = a->gameState->getGameTime();
      a->newVelocity->updateMagnitude(a->velocity);
      a->newAcceleration->updateMagnitude(a->acceleration);
      
      a->health -= b->getDamage(a);
   
      a->velocity->updateMagnitude(0, 0, 0);
      a->acceleration->updateMagnitude(0, 0, 0);

      a->energyHitAsteroid = true;
      b->shouldRemove = true;
      
      if (b->weapon->chargingShotid == b->id) {
         b->weapon->resetChargingShot();
      }
   }
   SoundEffect::playSoundEffect("BlasterHit.wav", b->position);
}

template<>
void Collision<Asteroid3D, TimedBombShot>::handleCollision() {
   Vector3D positionToAsteroid(*a->position, *b->position);
   double distance = positionToAsteroid.getLength();
   if (!b->isExploded) {
      if (distance > b->collisionRadius + a->radius) {
         //Vector3D* attraction = new Vector3D(positionToAsteroid);
         //attraction->setLength(5.0);
         //b->addAcceleration(attraction);
         //a->velocity->updateMagnitude(attraction);
         //attraction = new Vector3D(b->velocity);
         //attraction->setLength(-100.0);
         //b->addAcceleration(attraction);
      } else {
         if (a->gameState->gsm != ClientMode)
            b->shouldExplode = true;
         // TODO: Use a better sound effect here.
         SoundEffect::playSoundEffect("BlasterHit.wav", a->position);
      }
   } else if (distance < b->explodeRadius + a->radius) {
      //printf("Distance is: %f\n", distance);
      if (a->gameState->gsm != ClientMode) {
         a->health -= b->getDamage(a);
         Vector3D* shotToAsteroid;
         shotToAsteroid = new Vector3D(*b->position, *a->position);
         double distance = shotToAsteroid->getLength() - a->radius;
         double newSpeed = 1000 / ((1 + (distance * distance) / b->explodeRadius + 1) * a->radius);
         shotToAsteroid->setLength(newSpeed);
         a->addInstantAcceleration(shotToAsteroid);
         a->lastHitShotOwner = b->owner;
      }
   }
}

template<>
void Collision<Asteroid3D, HomingMissileShot>::handleCollision() {
   if (!b->isExploded) {
      Vector3D positionToAsteroid(*b->position, *a->position);
      double distance = positionToAsteroid.getLength();
      if (distance < b->seekRadius + a->radius) {
         if (distance > b->collisionRadius + a->radius) {
            if (a->gameState->gsm != ClientMode) {
               Vector3D toAsteroidNorm(positionToAsteroid);
               toAsteroidNorm.normalize();         
              
               double forwardAmount = b->forward->dot(toAsteroidNorm);
            
               if (forwardAmount > .75) {
                  if (b->targetID == -1) {
                     b->targetID = a->id;
                  } else {
                     Object3D* target = a->gameState->custodian[b->targetID];
                     if(target == NULL) {
                        b->targetID = a->id;
                     } else {
                        Vector3D positionToExistingAsteroid(*b->position, *(target->position));
                        Vector3D toExistingAsteroidNorm(positionToExistingAsteroid);
                        toExistingAsteroidNorm.normalize();

                        double existingForwardAmount = b->forward->dot(toExistingAsteroidNorm);
                        if (forwardAmount > .95 && existingForwardAmount > .95) {
                           double otherDistance = positionToExistingAsteroid.getLength();
                           if (distance < otherDistance) {
                              b->targetID = a->id;
                           }
                        } else if (forwardAmount > existingForwardAmount) {
                           b->targetID = a->id;
                        }
                     }
                  }
               }
            }
         } else {
            if (a->gameState->gsm != ClientMode) {
               b->shouldExplode = true;
               a->health -= b->getDamage(a);
               a->lastHitShotOwner = b->owner;
            }
            b->hasDamaged = true;
            SoundEffect::playSoundEffect("BlasterHit.wav", b->position);
         }
      }
   } else {
      if (a->gameState->gsm != ClientMode) {
         Vector3D positionToAsteroid(*b->position, *a->position);
         double distance = positionToAsteroid.getLength();
         if (!(b->hasDamaged) && distance < b->collisionRadius + a->radius) {
            Vector3D* shotToAsteroid;
            shotToAsteroid = new Vector3D(*b->position, *a->position);
            double distance = shotToAsteroid->getLength() - a->radius;
            //printf("Asteroid's health is: %f\n", a->health);
            double newSpeed = 1 / ((1 + (distance * distance) / b->explodeRadius + 1) * a->radius);
            shotToAsteroid->setLength(newSpeed);
            a->addInstantAcceleration(shotToAsteroid);
         }
      }
   }
}

template<>
void Collision<Asteroid3D, Asteroid3D>::handleCollision() {
   if (a->gameState->gsm == ClientMode) {
      return;
   }

   double d = a->position->distanceFrom(*b->position);
   double maxR = std::max(a->radius, b->radius);
   Vector3D* pushOnA = new Vector3D(*b->position, *a->position);
   Vector3D* pushOnB = new Vector3D(pushOnA->scalarMultiply(-1));

   if (d <= maxR) {
      if (d == 0) {
         //printf("stroid is stuck\n");
         pushOnA->randomMagnitude();
      }
      pushOnA->setLength(0.1); // Units per sec per sec.

      *pushOnB = pushOnA->scalarMultiply(-1);

      a->addAcceleration(pushOnA);
      b->addAcceleration(pushOnB);
   }

   Vector3D reflectionAxis(*b->position, *a->position);
   reflectionAxis.normalize();

   double m1 = a->radius; // This Asteroid's Mass.
   double m2 = b->radius; // Other Asteroid's Mass.

   Vector3D* newVelocity_a = new Vector3D(*a->velocity);
   Vector3D* newVelocity_b = new Vector3D(*b->velocity);

   // Added the 0.2 to make stuff lose some energy.
   newVelocity_a->scalarMultiplyUpdate(m1 - m2);
   newVelocity_a->addUpdate(b->velocity->scalarMultiply(2 * m2));
   newVelocity_a->scalarMultiplyUpdate(1 / (m1 + m2 + 0.2));
   
   newVelocity_b->scalarMultiplyUpdate(m2 - m1);
   newVelocity_b->addUpdate(a->velocity->scalarMultiply(2 * m1));
   newVelocity_b->scalarMultiplyUpdate(1 / (m1 + m2 + 0.2));
   
   // We want to only add acceleration away from the other item.
   if (newVelocity_a->dot(*pushOnA) < 0) {
      newVelocity_a->scalarMultiplyUpdate(-1);
   }
   
   if (newVelocity_b->dot(*pushOnB) < 0) {
      newVelocity_b->scalarMultiplyUpdate(-1);
   }
   
   a->addInstantAcceleration(newVelocity_a);
   b->addInstantAcceleration(newVelocity_b);

   Vector3D* reverseVelocity = new Vector3D(*a->velocity);
   reverseVelocity->scalarMultiplyUpdate(-1);
   a->addInstantAcceleration(reverseVelocity);

   reverseVelocity = new Vector3D(*b->velocity);
   reverseVelocity->scalarMultiplyUpdate(-1);
   b->addInstantAcceleration(reverseVelocity);
}

template<>
void Collision<Shard, TractorBeamShot>::handleCollision() {
   const int numParticles = 1;

   // Set the new speed.
   Vector3D* TBshotToShip = new Vector3D(*a->position, *b->owner->position);
   TBshotToShip->setLength(1000 / sqrt((1 + TBshotToShip->getLength())));
   
   // Pull the shard in.
   if (a->gameState->gsm != ClientMode) {
      a->addAcceleration(TBshotToShip);
   }

   // Now do particles.
   Vector3D random;
   for (int i = 0; i < numParticles; ++i) {
      Point3D* particlePosition = new Point3D(*a->position);
      random.randomMagnitude();
      random.movePoint(*particlePosition);
      Vector3D* particleVelocity = new Vector3D(*particlePosition, *b->position);
      particleVelocity->setLength(10); // Speed of the particle.
      particleVelocity->addUpdate(*a->velocity);
      // Make this go toward the ship.
      TractorAttractionParticle::Add(particlePosition, particleVelocity, b->owner->position, a->gameState);
   }
}

template<>
void Collision<Shard, BeamShot>::handleCollision() {
   if (!b->hitYet && curFrame - 1 <= b->firstFrame) {
      if (a->gameState->gsm != ClientMode) {
         double speed = 80; // High speed from hard-hitting railgun.
         a->velocity->updateMagnitude(*b->position, *a->position);
         a->velocity->setLength(speed);
      }
      b->hitYet = true;
      b->hitItem = a;
      b->lastHitFrame = curFrame;
      b->drawLength = a->position->distanceFrom(*b->position);
   }
}

template<>
void Collision<Asteroid3D, Shard>::handleCollision() {
   if (a->gameState->gsm != ClientMode) {
      double speed = a->velocity->getLength();
      b->velocity->updateMagnitude(*a->position, *b->position);
      b->velocity->setLength(speed);
   }
}

template<>
void Collision<Shard, Shard>::handleCollision() {
   if (a->gameState->gsm != ClientMode) {
      Vector3D* pushOnA = new Vector3D(*b->position, *a->position);
      if (pushOnA->getComparisonLength() == 0) {
         pushOnA->randomMagnitude();
      }
      pushOnA->setLength(5);
      a->addAcceleration(pushOnA);
      b->addAcceleration(new Vector3D(pushOnA->scalarMultiply(-1)));
   }
}

template<>
void Collision<Shard, TimedBombShot>::handleCollision() {
   if (!b->isExploded) {
      Vector3D positionToShard(*b->position, *a->position);
      double distance = positionToShard.getLength();
      if (distance < b->collisionRadius + a->collisionRadius) {
         b->shouldExplode = true;
         SoundEffect::playSoundEffect("BlasterHit.wav", b->position);
      }
   } else {
      if (a->gameState->gsm != ClientMode) {
         Vector3D* shotToShard;
         shotToShard = new Vector3D(*b->position, *a->position);
         double distance = shotToShard->getLength();
         double newSpeed = 100 / ((distance * distance) / b->explodeRadius + 1);
         shotToShard->setLength(newSpeed);
         a->addInstantAcceleration(shotToShard);
      }
   }
}

template<>
void Collision<Shard, HomingMissileShot>::handleCollision() {
   if (!b->isExploded) {
      Vector3D positionToShard(*b->position, *a->position);
      double distance = positionToShard.getLength();
      if (distance < b->collisionRadius + a->collisionRadius) {
         b->shouldExplode = true;
         SoundEffect::playSoundEffect("BlasterHit.wav", b->position);
      }
   } else {
      if (a->gameState->gsm != ClientMode) {
         Vector3D* shotToShard;
         shotToShard = new Vector3D(*b->position, *a->position);
         double distance = shotToShard->getLength();
         double newSpeed = 100 / ((distance * distance) / b->explodeRadius + 1);
         shotToShard->setLength(newSpeed);
         a->addInstantAcceleration(shotToShard);
      }
   }
}

static std::map<unsigned, CollisionBase*>* collisionHandlers = NULL;

Custodian::Custodian(const GameState* _gameState) :
 gameState(_gameState) {
   shardCount = asteroidCount = 0;
   nextID = defaultNextID;
   
   if (collisionHandlers == NULL) {
      collisionHandlers = new std::map<unsigned, CollisionBase*>();
      // Just to make syntax nicer.
      std::map<unsigned, CollisionBase*>& ch = *collisionHandlers;
      
      // Collision for AsteroidShip
      ch[(1 << TYPE_ASTEROIDSHIP)] = new Collision<AsteroidShip, AsteroidShip>;
      ch[(1 << TYPE_ASTEROIDSHIP) | (1 << TYPE_ASTEROID3D)] = new Collision<AsteroidShip, Asteroid3D>;
      ch[(1 << TYPE_ASTEROIDSHIP) | (1 << TYPE_SHARD)] = new Collision<AsteroidShip, Shard>;
      ch[(1 << TYPE_ASTEROIDSHIP) | (1 << TYPE_BLASTERSHOT)] = new Collision<AsteroidShip, BlasterShot>;
      ch[(1 << TYPE_ASTEROIDSHIP) | (1 << TYPE_BEAMSHOT)] = new Collision<AsteroidShip, BeamShot>;
      ch[(1 << TYPE_ASTEROIDSHIP) | (1 << TYPE_ELECTRICITYSHOT)] = new Collision<AsteroidShip, ElectricityShot>;
      ch[(1 << TYPE_ASTEROIDSHIP) | (1 << TYPE_ENERGYSHOT)] = new Collision<AsteroidShip, EnergyShot>;
      ch[(1 << TYPE_ASTEROIDSHIP) | (1 << TYPE_HOMINGMISSILESHOT)] = new Collision<AsteroidShip, HomingMissileShot>;
      ch[(1 << TYPE_ASTEROIDSHIP) | (1 << TYPE_TIMEDBOMBSHOT)] = new Collision<AsteroidShip, TimedBombShot>;

      // Collision for Asteroid3D
      ch[(1 << TYPE_ASTEROID3D)] = new Collision<Asteroid3D, Asteroid3D>;
      ch[(1 << TYPE_ASTEROID3D) | (1 << TYPE_BLASTERSHOT)] = new Collision<Asteroid3D, BlasterShot>;
      ch[(1 << TYPE_ASTEROID3D) | (1 << TYPE_BEAMSHOT)] = new Collision<Asteroid3D, BeamShot>;
      ch[(1 << TYPE_ASTEROID3D) | (1 << TYPE_TRACTORBEAMSHOT)] = new Collision<Asteroid3D, TractorBeamShot>;
      ch[(1 << TYPE_ASTEROID3D) | (1 << TYPE_ELECTRICITYSHOT)] = new Collision<Asteroid3D, ElectricityShot>;
      ch[(1 << TYPE_ASTEROID3D) | (1 << TYPE_ENERGYSHOT)] = new Collision<Asteroid3D, EnergyShot>;
      ch[(1 << TYPE_ASTEROID3D) | (1 << TYPE_TIMEDBOMBSHOT)] = new Collision<Asteroid3D, TimedBombShot>;
      ch[(1 << TYPE_ASTEROID3D) | (1 << TYPE_HOMINGMISSILESHOT)] = new Collision<Asteroid3D, HomingMissileShot>;
      ch[(1 << TYPE_ASTEROID3D) | (1 << TYPE_SHARD)] = new Collision<Asteroid3D, Shard>;

      // Collisions for Shard
      ch[(1 << TYPE_SHARD)] = new Collision<Shard, Shard>;
      ch[(1 << TYPE_SHARD) | (1 << TYPE_TRACTORBEAMSHOT)] = new Collision<Shard, TractorBeamShot>;
      ch[(1 << TYPE_SHARD) | (1 << TYPE_BEAMSHOT)] = new Collision<Shard, BeamShot>;
      ch[(1 << TYPE_SHARD) | (1 << TYPE_TIMEDBOMBSHOT)] = new Collision<Shard, TimedBombShot>;
      ch[(1 << TYPE_SHARD) | (1 << TYPE_HOMINGMISSILESHOT)] = new Collision<Shard, HomingMissileShot>;
      // Used to have a generic shot handler.
   }
}

/**
 * Compares by minX in Object3D.
 * Returns true if first argument goes before the second argument.* Sorts nulls last.
 */
static bool compareByMinX (Object3D* item1, Object3D* item2) {
   // If the first item is null, the second item comes first.
   if (item1 == NULL)
      return false;
   // If the second item is null, the first item comes first.
   if (item2 == NULL)
      return true;
   return item1->minPosition->x < item2->minPosition->x;
}

/**
 * Compares by maxX in Object3D.
 * Returns true if first argument goes before the second argument.* Sorts nulls last.
 */
static bool compareByMaxX (Object3D* item1, Object3D* item2) {
   // If the first item is null, the second item comes first.
   if (item1 == NULL)
      return false;
   // If the second item is null, the first item comes first.
   if (item2 == NULL)
      return true;
   return item1->maxPosition->x < item2->maxPosition->x;
}

bool compareByDistance::operator() (CollisionBase* const& lhs, CollisionBase* const& rhs) const {
   return lhs->squaredDistance < rhs->squaredDistance;
}
Object3D* compareByDistance::curObject;

/**
 * Run this after all objects have moved.
 * This sorts all the lists, then calculates each item's rank.
 * All NULL items are pushed to the end of the lists, where 
 * they are removed from the list.
 */
void Custodian::update() {
   Object3D* tempObject;
   // Iterate over objects that need to be added.
   while (!objectsToAdd.empty()) {
      // Get the first item in the list.
      tempObject = objectsToAdd.front();
      if (tempObject == NULL) {
         printf("Trying to insert null object.\n");
         objectsToAdd.pop_front();
         continue;
      }
      // Run the update function to make sure everything is right.
      tempObject->update(0);
      // Add the item to the sorted lists.
      if (!tempObject->shouldRemove) {
         objectsByMinX.push_back(tempObject);
         objectsByMaxX.push_back(tempObject);
      }
      // Remove the item.
      objectsToAdd.pop_front();
   }

   // This lets us know where we can stop searching.
   size_t firstNull = objectsByMinX.size();

   // Remove items that should be removed.
   for (unsigned i = 0; i < objectsByMinX.size(); ++i) {
      if (objectsByMinX[i] == NULL)
         continue;
      if (objectsByMinX[i]->shouldRemove) {
         remove(objectsByMinX[i]);
      }
   }

   // Sort. Duh.
   sort(objectsByMinX.begin(), objectsByMinX.end(), compareByMinX);
   sort(objectsByMaxX.begin(), objectsByMaxX.end(), compareByMaxX);

   // Set up ranks in objects.
   for (unsigned i = 0; i < objectsByMinX.size(); ++i) {
      if (objectsByMinX[i] == NULL) {
         firstNull = i;
         break;
      }
      objectsByMinX[i]->minXRank = i;
      objectsByMaxX[i]->maxXRank = i;
   }

   // Remove null pointers from the ends.
   for (size_t i = firstNull; i < objectsByMinX.size(); ++i) {
      objectsByMinX.pop_back();
      objectsByMaxX.pop_back();
   }
   
   assert (objectsByMinX.size() == objectsByMaxX.size());
}

/**
 * Call this on a Object3D object if you want it to be custodian'd.
 * This doesn't add the item right away, but waits until the next udpate 
 * to do it. In effect, items are added to a queue of items to add.
 */
void Custodian::add(Object3D* objectIn) {
   Object3D* existingObject = (*this)[objectIn->id];
   if (existingObject != NULL) {
      printf("Object %d exists already in custodian!\n", objectIn->id);
      return;
   }
   /* Sometimes, if we don't do this, we get weird situations where
      the client will add its own objects with the same ID as server objects.
      This should be fixed by never calling custodian->add on the client side.
      */
   nextID = std::max(nextID, objectIn->id + 1);
   objectsByID.insert(std::pair<unsigned, Object3D*>(objectIn->id, objectIn));

   objectsToAdd.push_back(objectIn);

   Asteroid3D* asteroid = NULL;
   Shard* shard = NULL;
   Shot* shot = NULL;
   AsteroidShip* ship = NULL;

   if (IS_SHOT(objectIn->type) && (shot = dynamic_cast<Shot*>(objectIn)) != NULL) {
      shots.insert(shot);
   } else if (objectIn->type == TYPE_ASTEROID3D &&
    (asteroid = dynamic_cast<Asteroid3D*>(objectIn)) != NULL) {
      asteroidCount++;
      asteroids.insert(asteroid);
   } else if (objectIn->type == TYPE_SHARD &&
    (shard = dynamic_cast<Shard*>(objectIn)) != NULL) {
      shardCount++;
      shards.insert(shard);
   } else if (objectIn->type == TYPE_ASTEROIDSHIP &&
    (ship = dynamic_cast<AsteroidShip*>(objectIn)) != NULL) {
      ships.insert(ship);
   }
}

/**
 * Iterate through objectsByID and remove any with shouldRemove set.
 * This should be called after each frame, after the server has sent updates.
 * We check isRemoved, which means that the custodian must have previously
 * called setRemoved() on this object. This is different from setting
 * shouldRemove, since the server may set shouldRemove, which tells the
 * client to remove it, and then the client actually does the removal.
 * The server cannot directly remove an object.
 */
void Custodian::cleanup() {
   std::map<unsigned, Object3D*>::iterator iter = objectsByID.begin();
   while (iter != objectsByID.end()) {
      if (iter->second->isRemoved()) {
         // DEBUG
         // printf("Deleting %d\n", iter->second->id);
         delete iter->second;
         objectsByID.erase(iter++);
      } else {
         iter++;
      }
   }
}

/**
 * Remove a Object3D object.
 * This is a private function. To make an object not be in the custodian, set
 * the object's shouldRemove = true.
 */
void Custodian::remove(Object3D* objectIn) {
   if (objectIn->isRemoved()) {
      // Looks like this got removed, so let's not diggity do it again.
      return;
   }

   // If we remove an object before it has been sorted, we get weird shits.
   if (objectsByMinX[objectIn->minXRank] == objectIn) {
      objectsByMinX[objectIn->minXRank] = NULL;
   }

   if (objectsByMaxX[objectIn->maxXRank] == objectIn) {
      objectsByMaxX[objectIn->maxXRank] = NULL;
   }

   // We're going to make the removal a separate step so that we can send
   // stuff from server to client.
   // objectsByID.erase(objectIn->id);

   // DEBUG
   // printf("Removing %d\n", objectIn->id);
   
   Asteroid3D* asteroid;
   Shard* shard;
   Shot* shot;
   AsteroidShip* ship;

   if (IS_SHOT(objectIn->type) &&
    (shot = dynamic_cast<Shot*>(objectIn)) != NULL) {
      shots.erase(shot);
   } else if (objectIn->type == TYPE_ASTEROID3D && 
    (asteroid = dynamic_cast<Asteroid3D*>(objectIn)) != NULL) {
      asteroidCount--;
      asteroids.erase(asteroid);
   } else if (objectIn->type == TYPE_SHARD && 
    (shard = dynamic_cast<Shard*>(objectIn)) != NULL) {
      shardCount--;
      shards.erase(shard);
   } else if (objectIn->type == TYPE_ASTEROIDSHIP &&
    (ship = dynamic_cast<AsteroidShip*>(objectIn)) != NULL) {
      ships.erase(ship);
   }

   // Last wishes? We don't use the destructor, since we might actually erase later.
   objectIn->onRemove();
   objectIn->setRemoved();

   // delete objectIn;
}

/**
 * Get a collision object for two objects that we know collide.
 */
CollisionBase* Custodian::getCollision(Object3D* a, Object3D* b) {
   if (a == NULL || b == NULL) {
      // DEBUG
      if (a == NULL)
         printf("a is null.\n");
      if (b == NULL)
         printf("b is null.\n");

      return NULL;
   }

   CollisionBase* curCollision = NULL;
   std::map<unsigned, CollisionBase*>::iterator curCollisionHandler;
   unsigned collisionHandlerKey;
   
   collisionHandlerKey = (1 << a->type) | (1 << b->type);
   curCollisionHandler = collisionHandlers->find(collisionHandlerKey);
   if (curCollisionHandler != collisionHandlers->end()) {
      curCollision = curCollisionHandler->second->tryExecute(a, b);
   }
   return curCollision;
}

/**
 * This returns a pointer to a new std::list. You must delete
 * if after you use it.
 */
std::set<CollisionBase*, compareByDistance>* Custodian::findCollisions(Object3D* item, bool searchBackwards) {

   compareByDistance::curObject = item;
   // TODO: Order this by distance.
   std::set<CollisionBase*, compareByDistance >* sublist = new std::set<CollisionBase*, compareByDistance>();
   
   CollisionBase* curCollision;
   std::map<unsigned, CollisionBase*>::iterator curCollisionHandler;

   // If we were passed a null pointer, don't worry about it, just return an empty list.
   if (item == NULL)
      return sublist;

   std::set<Object3D*, compareByDistance >::iterator iter;
   size_t numElements = objectsByMinX.size();
   Object3D* other;

   if (searchBackwards) {
      /* Start at the current maxX - 1. Check whether elements to 
       * the left have a maxX that is above item's minX.
       */
      for (int i = item->maxXRank - 1; i > 0; --i) {
         other = objectsByMaxX[i];
         if (other == NULL)
            continue;
         if (other->maxPosition->x >= item->minPosition->x) {
            curCollision = getCollision(item, other);;

            if (curCollision != NULL) {
               sublist->insert(curCollision);
            }
         }
         else break;
      }
   }
   
   /* Start at the current minX + 1. Check whether elements to
    * the right have a minX that is below item's maxX.
    */
   for (unsigned i = item->minXRank + 1; i < numElements; ++i) {
      other = objectsByMinX[i];
      if (other == NULL)
         continue;

      if (other->minPosition->x <= item->maxPosition->x) {
         curCollision = getCollision(item, other);;
            
         if (curCollision != NULL) {
            sublist->insert(curCollision);
         }
      } else {
         break;
      }
   }

   return sublist;
}

std::vector<Object3D*>* Custodian::getListOfObjects() {
   return &objectsByMinX;
}

void Custodian::clear() {
   objectsByMinX.clear();
   objectsByMaxX.clear();
   objectsByID.clear();
   objectsToAdd.clear();
   shipsByClientID.clear();

   ships.clear();
   asteroids.clear();
   shards.clear();
   shots.clear();

   asteroidCount = 0;
   shardCount = 0;
   nextID = defaultNextID;
}

/**
 * Return the current value of nextID and then increment nextID.
 */
unsigned Custodian::getNextID() {
   return nextID++;
}

/**
 * We want to return a null pointer if we search for a bad value.
 * I'm also going to add a debug output, but if anyone needs
 * the debug output, you can remove it.
 */
Object3D* Custodian::operator[] (unsigned i) { 
   std::map<unsigned, Object3D*>::iterator iter;
   iter = objectsByID.find(i);
   if (iter == objectsByID.end())
      return NULL;
   else
      return iter->second;
}

/**
 * Pass in a pointer to an entity (protocol buffer style).
 * This should update what's already in the list or make a new 
 * item with the given properties.
 */
Object3D* Custodian::updateObjectFromEntity(const ast::Entity& ent) {
   unsigned id = ent.id();
   unsigned type = ent.type();
   Object3D* obj = (*this)[id];
   if (obj == NULL) {
      // Object doesn't exit.
      if (ent.id() < nextID) {
         // Must have deleted already. Ignore,
         // DEBUG
         // printf("Tried to add deleted object %u.\n", ent.id());
         return NULL;
      }

      AsteroidShip* owner = NULL;
      if (IS_SHOT(type)) {
         
         if (ent.has_ownerid()) {
            // Should this be dynamic cast?
            owner = static_cast<AsteroidShip*>((*this)[ent.ownerid()]);
         } else {
            std::cout << "No owner specified!\n";
         }

         if (owner == NULL) {
            std::cout << "Could not find shot's owner!\n";
            exit(EXIT_FAILURE);
         }
      }

      switch (type) {
         case TYPE_ASTEROID3D:
            obj = new Asteroid3D(ent.radius(), gameState->worldSize, gameState, false);
            obj->load(ent);
            //std::cout << "Found Asteroid.\n";
         break;
         case TYPE_ASTEROIDSHIP:
            obj = new AsteroidShip(gameState);
            obj->load(ent);
            //std::cout << "Found Ship.\n";
         break;
         case TYPE_SHARD:
            obj = new Shard(0.5, gameState->worldSize, gameState);
            obj->load(ent);
            //std::cout << "Found Shard.\n";
         break;
         case TYPE_BEAMSHOT:
         {
            Point3D pos;
            Vector3D dir;
            pos.load(ent.position());
            dir.load(ent.velocity());

            obj = new BeamShot(pos, dir, ent.weaponindex(), owner, gameState);
            obj->load(ent);
            //std::cout << "Found Beamshot.\n";
         }
         break;
         case TYPE_BLASTERSHOT:
         {
            Point3D pos;
            Vector3D dir;
            pos.load(ent.position());
            dir.load(ent.velocity());

            obj = new BlasterShot(pos, dir, ent.weaponindex(), owner, gameState);

            obj->load(ent);
         }
         break;
         case TYPE_ELECTRICITYSHOT:
         {
            Point3D pos;
            Vector3D dir;
            pos.load(ent.position());
            dir.load(ent.velocity());
            
            obj = new ElectricityShot(pos, dir, ent.weaponindex(), owner, gameState);

            obj->load(ent);
            //std::cout << "Found Electricity shot.\n";
         }
         break;
         case TYPE_ENERGYSHOT:
         {
            Point3D pos;
            Vector3D dir;
            pos.load(ent.position());
            dir.load(ent.velocity());
            
            obj = new EnergyShot(pos, dir, ent.weaponindex(), owner, gameState);

            obj->load(ent);
            //std::cout << "Found Energyshot.\n";
         }
         break;
         case TYPE_HOMINGMISSILESHOT:
         {
            Point3D pos;
            Vector3D dir;
            pos.load(ent.position());
            dir.load(ent.velocity());
            
            obj = new HomingMissileShot(pos, dir, ent.weaponindex(), owner, gameState);

            obj->load(ent);
            // std::cout << "Found Homing Missile shot.\n";
         }
         break;
         case TYPE_TIMEDBOMBSHOT:
         {
            Point3D pos;
            Vector3D dir;
            pos.load(ent.position());
            dir.load(ent.velocity());
            
            obj = new TimedBombShot(pos, dir, ent.weaponindex(), owner, gameState);

            obj->load(ent);
            //std::cout << "Found Timed Bomb shot.\n";
         }
         break;
         case TYPE_TRACTORBEAMSHOT:
         {
            Point3D pos;
            Vector3D dir;
            pos.load(ent.position());
            dir.load(ent.velocity());
            
            obj = new TractorBeamShot(pos, dir, ent.weaponindex(), owner, gameState);

            obj->load(ent);
            //std::cout << "Found Tractor Beam shot.\n";
         }
         break;
         default:
         std::cout << "Found other object.\n";
      }
      if (obj != NULL) {
         add(obj);
         // std::cout << "Adding object " << obj->id << ".\n";
      } else {
         std::cout << "NULL OBJECT!\n";
      }
   } else {
      if (!obj->isRemoved())
         obj->load(ent);
   }
   //std::cout << "ID: " << id << std::endl;
   return obj;
}

const std::map<unsigned, Object3D*>& Custodian::getObjectsByID() {
   return objectsByID;
}
