/**
 * Object3D
 * Sterling Hirsh / Taylor Arnicar
 * 1-16-11
 * CPE 476
 */

#ifndef __OBJECT3D_H__
#define __OBJECT3D_H__

#include "Items/Drawable.h"
#include "Utility/Point3D.h"
#include "Utility/Vector3D.h"
#include "Utility/GlobalUtility.h"
#include <queue>
#include <boost/serialization/base_object.hpp>

class Object3D : public Drawable {
   //public variables------------------------------
   public:
      Vector3D* acceleration;
      Vector3D* axis;
      Vector3D *up, *right, *forward;
      bool lockUpVector;
      double angle;
      double rotationSpeed;
      unsigned id;
      
      
   //private variables------------------------------
   private:
   
   
   //public functions------------------------------
   public:
      //constructor
      Object3D(const GameState* _gameState);
      //destructor
      virtual ~Object3D();
      
      // Virtual functions required by Drawable class
      virtual void update(double timeDifference);
      virtual void draw();
      virtual void drawGlow();
      virtual std::string serialize();
      
      // Virtual functions for classes that inherit Object3D
      virtual void setYawSpeed(double radiansPerSecond);
      virtual void setPitchSpeed(double radiansPerSecond);
      virtual void setRollSpeed(double radiansPerSecond);
      virtual void drawBoundingBox();
      virtual void drawBoundingSphere();
      virtual void setTargeted(bool a);
      virtual bool isTargeted();
      virtual void addAcceleration(Vector3D* newAccel);
      virtual void addInstantAcceleration(Vector3D* newAccel);
      virtual void nullPointers();
      virtual Point3D* sphereCollideWithRay(const Point3D& origin, Vector3D direction, double* hitDistance = NULL);
      
      void glRotate(bool doTranspose = true);
      
   //private functions------------------------------
   private:
   
   
   //protected variables------------------------------
   protected:
      double yawSpeed, pitchSpeed, rollSpeed;
      GLuint displayList;
      std::queue<Vector3D*> accelerations;
      std::queue<Vector3D*> instantAccelerations;
      bool targeted;
      
      
   //protected functions------------------------------
   protected:
      void yaw(double angle);
      void roll(double angle);
      void pitch(double angle);
      virtual void updateBoundingBox();
      virtual void updateAcceleration(double timeDiff);

   // Serialization
   public:
      template<class Archive> 
            void serialize(Archive & ar, const unsigned int version) {
         ar & boost::serialization::base_object<Drawable>(*this);
         ar & velocity;

         ar & acceleration;
         ar & axis;
         ar & up;
         ar & right;
         ar & forward;
         ar & lockUpVector;
         ar & angle;
         ar & rotationSpeed;
         ar & id;

         ar & yawSpeed;
         ar & pitchSpeed;
         ar & rollSpeed;
         ar & displayList;
         ar & accelerations;
         ar & instantAccelerations;
         ar & targeted;
      }
};

#endif
