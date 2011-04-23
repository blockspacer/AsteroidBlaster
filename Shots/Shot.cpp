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
 AsteroidShip* const ownerIn, const GameState* _gameState) : owner(ownerIn),
 Object3D(_gameState) {
   *position = posIn;
   velocity = new Vector3D(dirIn);
   //velocity->setLength(40.0);
   timeFired = doubleTime();
   isBeam = false;
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
void Shot::handleCollision(Drawable* other) {
   if (other == owner)
      return;

   // Don't remove persistent stuff.
   if (!persist) {
      shouldRemove = true;
   }
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

Point3D Shot::getWallIntersectionPoint(BoundingWall* wall) {
   if (!isBeam) {
      return Object3D::getWallIntersectionPoint(wall);
   }

   Point3D toReturn(*position);
   Point3D pointOnPlane = wall->normal.scalarMultiply(-wall->wallSize);

   // Calculate intersection between beam and wall.
   Vector3D normalizedDirection = velocity->getNormalized();
   Vector3D wallNormal = wall->normal.scalarMultiply(-1); // To make it face away from the center.
   double rayDotNormal = normalizedDirection.dot(wallNormal);
   if (rayDotNormal <= 0) {
      wall->actuallyHit = false;
      return toReturn;
   }

   Vector3D rayOriginToPointOnPlane(*position, pointOnPlane);

   double distance = rayOriginToPointOnPlane.dot(wallNormal) / rayDotNormal;
   if (distance < 0) {
      wall->actuallyHit = false;
      return toReturn;
   }

   toReturn.addUpdate(normalizedDirection.scalarMultiply(distance));
   return toReturn;
}

double Shot::getDamage(Object3D* collidedObject) {
   return damage;
}
