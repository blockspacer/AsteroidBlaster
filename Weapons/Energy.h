/**
 * Energy! The original Asteroids Weapon.
 * @author Sterling Hirsh (shirsh@calpoly.edu)
 * @date 2/14/2011
 * <3
 */

#ifndef __ENERGY_H__
#define __ENERGY_H__

#include "Weapons/Weapon.h"

struct Point3D;
/* Classter Energy */
class Energy : public Weapon {
   public:
      Energy(AsteroidShip* owner);
      virtual ~Energy();
      virtual Point3D project(Object3D*);
      virtual void update(double timeDiff);
      virtual bool shouldFire(Point3D*, Point3D*);
      void godMode(bool enabled);
      virtual void debug();
      virtual void fire();

   protected:
      double shotSpeed;
      double turnSpeed;
      double randomVariationAmount;
      Point3D* lastShotPos;
};

#endif