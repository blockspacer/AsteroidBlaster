/**
 * TimedBombShot.h
 * This bomb lasts for a few seconds before exploding.
 */

#ifndef __TIMEDBOMBSHOT_H__
#define __TIMEDBOMBSHOT_H__

#include "Shots/Shot.h"
#include "Shots/ExplosiveShot.h"

class TimedBombShot : public ExplosiveShot {
   public:
      TimedBombShot(Point3D &posIn, Vector3D dirIn, AsteroidShip* const ownerIn, const GameState* _gamestate);
      virtual ~TimedBombShot();
      virtual void draw();
      virtual void drawExplosion();
      virtual void update(double timeDiff);
      virtual void hitWall(BoundingWall* wall);
      // Called when the shot's isExploded becomes true.
      virtual void explode();

      // In seconds.
      double spin;
      double scaleSize;
      double secondScale;
      double addSize;
      double rx;
      double ry;
      double rz;
      double timeSinceExploded;
      double timeToExplode;
      double seekRadius;
      double slowDownPerSecond;
      double collisionRadius;
};

#endif

