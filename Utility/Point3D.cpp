/**
 * @file
 * Point3D implementation.
 *
 * @author Sterling Hirsh, Mike Smith
 */

#include "Utility/Point3D.h"
#include "Utility/Vector3D.h"
#include <math.h>

/** Doubles have a lot of precision.
 * We want to cut that down a bit.
 */
#define EPSILON 0.001

Point3D::Point3D(const Vector3D& copy) {
   x = copy.x;
   y = copy.y;
   z = copy.z;
}

const Point3D Point3D::Zero(0, 0, 0);

void Point3D::print() {
   printf("Point: (%f, %f, %f)\n", x, y, z);
}

