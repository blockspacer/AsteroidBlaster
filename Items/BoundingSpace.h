/**
 * Cube constrains objects' motion.
 * Sterling Hirsh / Taylor Arnicar
 * 1-20-11
 * CPE 476
 */

#include "Utility/Point3D.h"
#include "Utility/Object3D.h"

class BoundingSpace {
   public:
      double extent; // Only one var instead of min/max x, y, z
      double xMax, xMin, yMax, yMin, zMax, zMin;

      BoundingSpace(double extentIn, double x, double y, double z);
      ~BoundingSpace();
      virtual void constrain(Object3D* item);
};