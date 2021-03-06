/**
 * Camera!
 * Helps you look at stuff.
 * "Why don't you take a picture, it'll last longer."
 *   - Peewee's Big Adventure (And probably a lot of things)
 *
 * Sterling Hirsh / Taylor Arnicar
 * 1-17-11
 * CPE 476
 */

#ifndef __CAMERA_H__
#define __CAMERA_H__

#include "Utility/Point3D.h"
#include "Utility/Vector3D.h"
#include "Items/Object3D.h"

class Camera {
   public:
      Point3D* position;
      Vector3D *velocity;
      Vector3D *up, *right, *forward;
      Vector3D *offset;
      bool lockUpVector;
      float shakeAmount;
      
      double converganceDistance;
      double stereoOffsetAmount;

      Camera(bool lockUp);
      Camera(Object3D*);
      virtual ~Camera();
      void setCamera(bool setPosition = true);
      void setCamera(Point3D* newPosition, Vector3D* upIn, 
       Vector3D* rightIn, Vector3D* forwardIn);
      void viewFrom(Object3D* object);
      void setOffset(double x, double y, double z);
      void setOffset(Vector3D& newOffset);
      Point3D getEyePoint();
      void shake(float newShakeAmount);
      void setViewVector(Vector3D* newView);
      void lookAt(double x, double y, double z);
};

#endif
