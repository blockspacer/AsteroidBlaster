/**
 * @file
 * The Shooting AI class implementation.
 *
 * <pre>
 * The Shooting AI is in charge of controlling the ships weapons independently
 * of the flyer.
 * The Shooting AI shall:
 *    * Acquire targets quickly
 *    * Use information about targets to choose the best target.
 *    * Destroy targets
 *    * Help the ship stay alive
 *    * Keep the game interesting by simulating human inaccuracies.
 *
 * @author Mike Smith, Taylor Arnicar, Sean Ghiocel, Justin Kuehn
 */

#include "AI/ShootingAI.h"
#include "Items/Asteroid3D.h"

const double ShootingAI::gunRotSpeed = 1.0;

ShootingAI::ShootingAI(AsteroidShip* owner)
{
   ship = owner;
   aimingAt = Point3D(0,1,0);
   lastShotPos = Point3D(0,0.9,0);
   // Start the AI as disabled
   enabled = false;
   //TODO possibly more stuff here.
}

/** Aims at an object.
 *
 * This function should be called multiple frames consecutively in order
 * to actually aim at the object. The gun barrel has a rotation time,
 * and this function takes care of moving the virtual barrel to where it
 * needs to be. This will take care of predicting where the target will
 * be based on its current velocity.
 *
 * @param dt time difference since last frame, for gun turning.
 * @return no idea, really. Just leaving this open in case I think of something
 * TODO: Make this work. I (Sterling) was getting huge values for wouldHit.
 * This is almost certainly a bug.
 */
int ShootingAI::aimAt(double dt, Object3D* target)
{
   return 0;
   Point3D wouldHit;
   double speed = 20;//chosenWeapon->getSpeed();
   double time = 0;
   double dist;
   double len;
   // Change in position
   Vector3D dp;
   
   Point3D curTarget = *target->position;
   // This section of code does angle interpolation.
   // Calculate the vector that points from our current direction to where
   // we want to be pointing.
   wouldHit = lastShotPos - aimingAt;
   printf("In aimAt:\n");
   wouldHit.print();

   // If the difference is more than the radius of the target,
   // we need to adjust where we are aiming.
   /*
   if (wouldHit.magnitude() > gunRotSpeed*dt) {
      // Normalize the vector.
      wouldHit = wouldHit.normalize();

      // Scale with the max gun rotation speed
      // multiplied with the amount of time that passed since the last frame
      wouldHit = wouldHit * (gunRotSpeed * dt);

      // calculate the vector that points from our position
      // to the spot that we want to be aiming. Normalize it as it is a 
      // direction.
      aimingAt = (wouldHit - *ship->position).normalize();
   }
   */

   // This loop will choose the spot that we want to be shooting at.
   do {
      // time is the distance from the ship to the target according to the
      // speed of the bullet.
      time = ship->position->distanceFrom(curTarget) / speed;

      // dp is the distance the asteroid traveled in the time it took for our
      // bullet to get to the point where the asteroid was.
      dp = target->velocity->scalarMultiply(time);

      // Move our target to the point where the asteroid would be
      dp.movePoint(curTarget);

      // Get the vector that points to that point.
      wouldHit = curTarget - *ship->position;

      // Normalize, scale by speed of the bullet multiplied by the time that
      // passed (for the bullet to get to the previous point) This vector
      // now points to where our bullet will be when the asteroid is at
      // its position
      wouldHit = wouldHit.normalize() * speed * time + *ship->position;
      printf("wouldHit: ");
      wouldHit.print();

      // Dist is the distance from where our bullet will be to where
      // the asteroid will be.
      dist = wouldHit.distanceFrom(curTarget);

      // If this distance is greater than the radius (ie, we missed),
      // recalculate!
   } while (dist > target->radius);

   // By the end of the loop, curTarget is the point that we need to aim
   // at in order to hit our target.
   lastShotPos = (curTarget - *ship->position).normalize();
   aimingAt = lastShotPos;
   printf("aimingAt: ");
   aimingAt.print();
   ship->updateShotDirection(aimingAt);

   return 0;
}

// kinda useless right now.
void ShootingAI::chooseWeapon( int weapon )
{
    ship->selectWeapon( weapon );
    //chosenWeapon = ship->getWeapon( weapon );
}

Object3D* ShootingAI::chooseTarget()
{

   std::vector<Object3D*> *targets = ship->getRadar()->getFullReading();

   std::vector<Object3D*>::iterator targets_iterator;

   Point3D* ship_position = ship->position;

   targets_iterator = targets->begin();
   Object3D* closest = *targets_iterator;
   double shortest_distance = closest->position->distanceFrom( *ship_position );
   
   double distance;
   Asteroid3D* asteroid;
   for ( ; targets_iterator != targets->end(); targets_iterator++ ) {
      // get closest asteroid
      //if (*targets_iterator == NULL || *targets_iterator == ship || NULL == (asteroid = dynamic_cast<Asteroid3D*>(*targets_iterator)) || asteroid->isShard  )
      // Trying this with shard subclass
      // IF THE AI BREAKS, REMOVE THIS
      if (*targets_iterator == NULL || *targets_iterator == ship || NULL == (asteroid = dynamic_cast<Asteroid3D*>(*targets_iterator)) || dynamic_cast<Shard*>(*targets_iterator) != NULL)
      // END OF NEW CODE
         continue;
      
      distance = (*targets_iterator)->position->distanceFrom( *ship_position );
      if ( distance < shortest_distance )
      {
         shortest_distance = distance;
         closest = *targets_iterator;
      } 
   }

    return closest;
}

int ShootingAI::think(double dt)
{
   if(!enabled) {
      return 0;
   }
   
   // TODO this is where stuff goes! Should probably figure out the order of
   // the functions.

   /* Also, do we want to try and rechoose a target every think (ie frame)?
    * If not, it would make the shooter a little quicker, but may end up
    * shooting at bad targets (if the gun is pointed in the opposite direction,
    * it will take time to rotate it around to shoot the target, then
    * time to kill the asteroid, and throughout all this time a new asteroid
    * could have become a better target, or even a critical target)
    *
    * Just an idea, we will talk it over.
    */

   /* This order is tentative. We will probably change it. */
   
   // choose weapon railgun
   chooseWeapon(0);

   Object3D* target = chooseTarget();
   // choose target

   aimAt(dt, target);
   ship->fire(true);

   // Think has a return value just in case it needs to.
   return 0;
}

void ShootingAI :: enable() {
   AI :: enable();
}

void ShootingAI :: disable() {
   AI :: disable();
   ship->fire(false);
}
