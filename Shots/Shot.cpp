/**
 * Shot.cpp
 * Sterling Hirsh
 * A shot in 3D Asteroids.
 */

#include "Shots/Shot.h"
#include <iostream>

materialStruct ShotMaterial = {
  {1, 0, 0, 1},
  {1, 0, 0, 1},
  {1, 0, 0, 1},
  {8.0}
};

Shot::Shot(Point3D& posIn, Vector3D dirIn,
 AsteroidShip* const ownerIn) : owner(ownerIn),
 Object3D(0, 0, 0, 0) {
   *position = posIn;
   velocity = new Vector3D(dirIn);
   //velocity->setLength(40.0);
   timeFired = doubleTime();
   lifetime = 2;
   
}

Shot::~Shot() {
   // Do nothing.
}
      

void Shot::update(double timeDiff) {
   Object3D::update(timeDiff);
   if (doubleTime() - timeFired > lifetime) {
      shouldRemove = true;
   }
}

/**
 * Ignore collisions with the owner's ship.
 */
void Shot::handleCollision(Object3D* other) {
   if (other == owner)
      return;
   shouldRemove = true;
}

void Shot::draw() {
   Object3D::draw();
}

/**
 * Don't draw shots in the map.
 * For now :)
 */
void Shot::drawInMinimap() {
   return;
}

void Shot::debug() {
   printf("Shot: (min, max, position, velocity)\n");
   minPosition->print();
   maxPosition->print();
   position->print();
   velocity->print();
   printf("--\n");
}