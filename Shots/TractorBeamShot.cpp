/**
 * TractorBeamShot.cpp
 * Incredibly attractive shots.
 * Vwahhhh
 * @author Sterling Hirsh (shirsh@calpoly.edu)
 * @date Valentine's Day
 * <3
 */

#include "Items/Shard.h"
#include "Shots/TractorBeamShot.h"
#include <math.h>

#ifdef WIN32
#include "Utility/WindowsMathLib.h"
#endif

static float spin = 0;

TractorBeamShot::TractorBeamShot(Point3D& posIn, Vector3D dirIn, int _weaponIndex,
 AsteroidShip* const ownerIn, const GameState* _gameState) : Shot(posIn, dirIn, _weaponIndex, ownerIn, _gameState) {
   type = TYPE_TRACTORBEAMSHOT;
   persist = true;
   angle = M_PI / 20; // Radians from the center
   // The length of the tractor beam shot is the range of the weapon.
   length = owner->weapons[TRACTOR_WEAPON_INDEX]->range + (owner->weapons[TRACTOR_WEAPON_INDEX]->level * 5);
   farRadius = length * tan(angle);

   forward = new Vector3D(*velocity);
   up = new Vector3D(*ownerIn->up);

   setPosAndDir(posIn, dirIn);


   shouldConstrain = false;
   // Make sure tractor beam shots aren't culled from the view frustum.
   // (necessary to make them appear)
   shouldBeCulled = false;
   damage = 0;

   collisionType = new CollisionCone(angle, length, *velocity, *position);
}

/**
 * Kill this after a certain number of frames.
 */
void TractorBeamShot::update(double timeDiff) {
   const float spinSpeed = 90; // Degrees per second.
   spin += (float) (spinSpeed * timeDiff * 2);
}

/**
 * We want to remove this after drawing it once.
 */
void TractorBeamShot::draw() {
   
   glPushMatrix();
      glUseProgram(tractorBeamShader);
      glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
      //floats used in loop iteration
      float j;
      float k;
      glDisable(GL_CULL_FACE);
      glDisable(GL_POLYGON_OFFSET_LINE);
      //rotation for the Y value of the vertex using cosine
      float rot;
      
      //rotation for the X value of the vertex using sine
      float srot;
      
      //Number of lines you want generated in the rotating tractor beam
      float numCurves = 8;
      
      //used as a multiplier to create the correct gap in between the lines
      float gap = (float) length / numCurves;
      
      //must be 2 for the cone to be complete, less than 2 for an incomplete cone, more 
      //than 2 for spacing to be irregular between the lines
      float sharpness = 2;
      
      Point3D start(*position);
      velocity->movePoint(start, length);
      start.glTranslate();
      
      glRotate();
      glRotatef(-spin,0,0,1);
      if (spin >= 360 ) {
         spin = 0;
      }
      //setMaterial(GreenShiny);
      glLineWidth(3.0);
      glBegin(GL_LINES);
      //Creates the twisted lines whose vertices are sent to the shader to modify
      for(k = -numCurves/2; k < numCurves/2; k = k+1){
            for(j = 0; j < length ; j = j+1){
                  rot = (float) cos((k * gap + j) / (length / sharpness) * M_PI);
                  srot = (float) sin((k * gap + j) / (length / sharpness) * M_PI);
                  glVertex3d(srot * j * (farRadius / length), rot * j * (farRadius / length), length - j);
                  
                  rot = (float) cos((k * gap + j + 1) / (length / sharpness) * M_PI);
                  srot = (float) sin((k * gap + j + 1) / (length / sharpness) * M_PI);
                  glVertex3d(srot * (j + 1) * (farRadius / length), rot * (j + 1) * (farRadius / length), length - j - 1);
                  
            }
      }
   glEnd();

   glLineWidth(1.0);
   
   glUseProgram(0);
   glUseProgram(tractorFade);
   
   //glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
      //glBlendFunc(GL_ONE, GL_ONE_MINUS_SRC_ALPHA);
      //glDisable(GL_LIGHTING);
      glBegin(GL_TRIANGLE_FAN);

    glColor4d(0, 1, 0, .2);
    glVertex3d(0.0f, 0.0f, length);
    float angle;
    double x, y, z;
    for(angle = 0.0f; angle < (2.0f*M_PI); angle += (float) (M_PI/16.0f))

        {

           // Calculate x and y position of the next vertex

           x = (farRadius * sin(angle));

           y = (farRadius * cos(angle));

           z = 0;

           glVertex3d(x, y, z);

        }
        
        x = farRadius * sin(angle);

        y = farRadius * cos(angle);

        z = 0;
        
        glVertex3d(x, y, z);

   glEnd();
   glEnable(GL_LIGHTING);
   glEnable(GL_CULL_FACE);
   glLineWidth(1.0);
   glUseProgram(0);
   glEnable(GL_POLYGON_OFFSET_LINE);
   glPopMatrix();

   glLineWidth(1.0);
}

void TractorBeamShot::setPosAndDir(Vector3D& newPos, Vector3D& newDir) {
   position->clone(&newPos);
   velocity->clone(&newDir); // It should be a unit vector.
   forward->clone(velocity);

   Point3D endPoint1(*position);
   Point3D endPoint2(*position);

   // Set endPoint2 100 units away.
   velocity->movePoint(endPoint2, length);
   // Correct for position when calculating endpoint1 and 2.
   Vector3D positionVector(*position);
   positionVector = positionVector.scalarMultiply(-1);
   positionVector.movePoint(endPoint1);
   positionVector.movePoint(endPoint2);
   
   // Now set min/max xyz
   minX = std::min(endPoint1.x, endPoint2.x);
   maxX = std::max(endPoint1.x, endPoint2.x);
   minY = std::min(endPoint1.y, endPoint2.y);
   maxY = std::max(endPoint1.y, endPoint2.y);
   minZ = std::min(endPoint1.z, endPoint2.z);
   maxZ = std::max(endPoint1.z, endPoint2.z);
   
   // Expand the bounding box to make sure it contains the whole cone.
   minX -= farRadius;
   minY -= farRadius;
   minZ -= farRadius;
   maxX += farRadius;
   maxY += farRadius;
   maxZ += farRadius;
   updateBoundingBox();
}

void TractorBeamShot::save(ast::Entity* ent) {
   Shot::save(ent);
}

void TractorBeamShot::load(const ast::Entity& ent) {
   Shot::load(ent);
   if (gameState->gsm == ClientMode && owner == gameState->ship) {
      velocity->updateMagnitude(owner->shotDirection);
   }

   setPosAndDir(*position, *velocity);
}

