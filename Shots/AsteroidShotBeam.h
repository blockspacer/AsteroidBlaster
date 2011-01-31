/**
 * AsteroidShotBeam.h
 * Sterling Hirsh
 * Shot type 2
 */

#ifndef __ASTEROIDSHOTBEAM_H__
#define __ASTEROIDSHOTBEAM_H__

#include "AsteroidShot.h"

class AsteroidShotBeam : public AsteroidShot {
   public:
      static double frequency; // Shots per sec
      bool hitYet;
      unsigned long lastHitFrame;

      AsteroidShotBeam(Point3D& posIn, Vector3D dirIn, AsteroidShip* const ownerIn);
      virtual void draw();
      virtual bool checkHit(Object3D* other);
};


#endif
