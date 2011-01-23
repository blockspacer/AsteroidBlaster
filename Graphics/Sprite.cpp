/**
 * Sterling Hirsh
 * Texture Importer
 * 11/29/2010
 * This loads an image and can move through parts of it on a rectangle.
 */

#include "Graphics/Sprite.h"
#include "Utility/Vector3D.h"
#include <math.h>

using namespace std;

list<Sprite*> Sprite::sprites;

Sprite::Sprite(std::string filename, int framesXIn, int framesYIn, double fpsIn, 
 Point3D posIn, double drawWidth, double drawHeight) : position(posIn) {
   framesX = framesXIn;
   framesY = framesYIn;
   framesPerSecond = fpsIn;
   width = drawWidth;
   height = drawHeight;
   frameWidth = 1.0 / framesX;
   frameHeight = 1.0 / framesY;
   texture = new TextureImporter(filename);
   materialStruct WhiteSolid = {
     {1, 1, 1, 1},
     {0.0, 0.0, 0.0, 0.0},
     {0.0, 0.0, 0.0, 0.0},
     {0.0}
   };
   curMaterial = WhiteSolid;
   totalFrames = framesX * framesY;
   startTime = doubleTime();
   oneShot = true;
}
      
bool Sprite::draw(Point3D* eyePoint) {
   // Do rotation and translation here.
   // Also update the current frame.
   
   Vector3D forward(0, 0, 1);
   Vector3D toCamera(position, *eyePoint);
   Vector3D cross = forward.cross(toCamera);

   double angle = forward.getAngleInDegrees(toCamera);

   double curTime = doubleTime();
   int curFrame = floor((curTime - startTime) * framesPerSecond);
   if (curFrame >= totalFrames && oneShot) {
      return false;
   }
   glDisable(GL_LIGHTING);
   glEnable(GL_TEXTURE_2D);
   glPushMatrix();
   position.glTranslate();
   glRotatef(angle, cross.xMag, cross.yMag, cross.zMag);
   
   double topLeftX = (curFrame % framesX) * frameWidth;
   double topLeftY = 1 - ((curFrame / framesX) * frameHeight);
   
   materials(curMaterial);
   glColor3f(1, 1, 1);
   glBindTexture(GL_TEXTURE_2D, texture->texID);
   
   glBegin(GL_QUADS);
   glTexCoord2f(topLeftX, topLeftY);
   glVertex3f(-width/2, height/2, 0);

   glTexCoord2f(topLeftX + frameWidth, topLeftY);
   glVertex3f(width/2, height/2, 0);

   glTexCoord2f(topLeftX + frameWidth, topLeftY - frameHeight);
   glVertex3f(width/2, -height/2, 0);

   glTexCoord2f(topLeftX, topLeftY - frameHeight);
   glVertex3f(-width/2, -height/2, 0);

   glEnd();
   glPopMatrix();
   glDisable(GL_TEXTURE_2D);
   glEnable(GL_LIGHTING);
   return true;
}
