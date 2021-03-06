/**
 * HomingMissile. Used as an example for other Explosive Weapon implementations.
 * @author Taylor Arnicar
 * @date 4/10/2011
 */

#ifndef __HOMINGMISSILE_H__
#define __HOMINGMISSILE_H__

#include "Weapons/Weapon.h"

class Point3D;

class HomingMissile : public Weapon {
   public:
      HomingMissile(AsteroidShip* owner, int _index);
      virtual ~HomingMissile();
      virtual Point3D project(Object3D*, Vector3D);
      virtual void update(double timeDiff);
      virtual bool shouldFire(Point3D*, Point3D*);
      void godMode(bool enabled);
      virtual void debug();
      virtual void fire();
      virtual double getCoolDownAmount();
   
   protected:
      float shotSpeed;
      float randomVariationAmount;
      Point3D* lastShotPos;
};

#endif
