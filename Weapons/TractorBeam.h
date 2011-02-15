/**
 * Tractor Beam
 * This pulls in crystals.
 * @author Sterling Hirsh (shirsh@calpoly.edu)
 * @date Valentine's Day <3
 */

#ifndef __TRACTORBEAM_H__
#define __TRACTORBEAM_H__

#include "Weapons/Weapon.h"
#include "Items/AsteroidShip.h"

class AsteroidShip;

class TractorBeam : public Weapon {
   public:
      TractorBeam(AsteroidShip* owner);
      virtual ~TractorBeam();
      virtual bool aimAt(Object3D*);
      virtual void update(double timeDiff);
      virtual void debug();
      virtual void fire();

};

#endif