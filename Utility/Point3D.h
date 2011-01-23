/**
 * Sterling Hirsh
 * CPE 471 
 * 10-19-10
 *
 * Point3D: A class to store a point in 3D space.
 */

#ifndef __POINT3D_H__
#define __POINT3D_H__

#include <stdio.h>
#include "Graphics/GlutUtility.h"

struct Point3D {
   double x, y, z;
   Point3D(double xIn = 0, double yIn = 0, double zIn = 0) :
    x(xIn), y(yIn), z(zIn) {}

   void clone(Point3D* other) {
      x = other->x;
      y = other->y;
      z = other->z;
   }

   void update(double x2, double y2, double z2) {
      x = x2;
      y = y2;
      z = z2;
   }
   
   double distanceFrom(Point3D& rhs) {
      return distance3D(rhs.x - x, rhs.y - y, rhs.z - z);
   }

   virtual void draw() {
      glVertex3f(x, y, z);
   }
   void print() {
      printf("(%f, %f, %f)\n", x, y, z);
   }
   void glTranslate() {
      glTranslatef(x, y, z);
   }
};

#endif