/**
 * HomingMissile. Used as an example for other Explosive Weapon implementations.
 * @author Taylor Arnicar
 * @date 4/10/2011
 */

#include "Weapons/HomingMissile.h"
#include "Utility/GlobalUtility.h"
#include "Shots/HomingMissileShot.h"
#include "Utility/Point3D.h"
#include "Utility/SoundEffect.h"
#include "Text/GameMessage.h"

HomingMissile::HomingMissile(AsteroidShip* owner, int _index) : Weapon(owner, _index) {
   HOMINGMISSILE_WEAPON_INDEX = index;
   shotSpeed = 12.5; // Units per second
   coolDown = .2; // Seconds
   randomVariationAmount = 0.25; // Units
   name = "Homing Missiles";
   lastShotPos = new Point3D(0, 1, 0);
   curAmmo = -1; // Infinite ammo
   purchased = false;

   fireBackwards = false; // This used to be true for mine layer.

   overheatLevel = 3;
   heatPerShot = 0.5;

   icon = "MissileIcon";
   r = 0.0;
   g = 0.0;
   b = 1.0;
}

HomingMissile::~HomingMissile() {
   delete lastShotPos;
}

/**
 * Called every frame.
 */
void HomingMissile::update(double timeDiff) {
   Weapon::update(timeDiff);
   if (isOverheated() && this->ship == ship->gameState->ship)
      GameMessage::Add("Homing Missiles overheated!", 30, 0, ship->gameState);
}

/**
 * This is what actually shoots. Finally!
 */
void HomingMissile::fire() {
   if (!isReady())
      return;

   // This isn't a big deal if it happens or not, but it might not.
   ship->setShakeAmount(0.01f);

   // If it's client mode, wait for the shot packet to arrive, 
   // and then add to the game.
   if (ship->gameState->gsm == ClientMode) {
      return;
   }
   static Vector3D randomVariation;
   //lastShotRight = !lastShotRight;
   //printf("Did it shoot right? %d\n", lastShotRight);
   // Update timeLastFired with new current time.
   timeLastFired = ship->gameState->getGameTime();
   // Copy the ship's position for the start point.
   Point3D start = ship->shotOrigin;
   // Copy the shot direction, set length to shotSpeed (since shotDirection is unit-length).
   shotSpeed *= -1;
   Vector3D shotDirection(ship->right->scalarMultiply(shotSpeed));
   // Add a random variation to each of the shots.
   randomVariation.randomMagnitude();
   randomVariation.scalarMultiplyUpdate(randomVariationAmount);
   shotDirection.addUpdate(randomVariation);
   ship->shotDirection.movePoint(start);
   ship->custodian->add(new HomingMissileShot(start,
            shotDirection, index, ship, ship->gameState));
   addHeat(heatPerShot / level);
}

/**
 * Basic debug function. Just in case!
 */
void HomingMissile::debug() {
   printf("%s\n", name.c_str());
}

/**
 * The job of the weapon is to project the position of
 * the targeted object, and return the point that the
 * AI should aim at in order to hit the target with this weapon.
 */
Point3D HomingMissile::project(Object3D* target, Vector3D addOn) {
   Point3D wouldHit;
   double time = 0, dist = 0;
   int iterations = 0;

   Point3D targetPos = *target->position;
   targetPos.addUpdate(addOn);
   Point3D curTarget = targetPos;
   Point3D dp;

   // This loop will choose the spot that we want to be shooting at.
   do {
      // time is the distance from the ship to the target according to the
      // speed of the bomb.
      time = ship->position->distanceFrom(curTarget) / shotSpeed;

      // dp is the distance the asteroid traveled in the time it took for our
      // bomb to get to the point we are considering (curTarget).
      dp = target->velocity->scalarMultiply(time);

      // Move our target to the point where the asteroid would be
      curTarget = dp + targetPos;

      // Get the vector that points to that point.
      wouldHit = curTarget - *ship->position;

      // Normalize, scale by speed of the bomb multiplied by the time that
      // passed (for the bomb to get to the previous point) This vector
      // now points to where our bomb will be when the asteroid is at
      // its position

      wouldHit = wouldHit.getNormalized() * shotSpeed * time + *ship->position;

      // Dist is the distance from where our bullet will be to where
      // the asteroid will be.
      dist = wouldHit.distanceFrom(curTarget);
      iterations++;
      // If this distance is greater than the radius (ie, we missed),
      // recalculate!
   } while (dist > target->radius && iterations < 5);

   return curTarget;
}

/**
 * Tell the Shooting AI whether or not it should fire at the target given the
 * target and where it's aiming.
 * This leads to the AI leading it shots.
 */
bool HomingMissile::shouldFire(Point3D* target, Point3D* aim) {
   return ((*target - ship->shotOrigin).getNormalized() - *aim).magnitude() < 0.5;
}

/**
 * This is for the weapon bar.
 */
double HomingMissile::getCoolDownAmount() {
   return 1 - clamp(currentHeat / overheatLevel, 0, 1);
}

