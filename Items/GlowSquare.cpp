/**
 * GlowSquare is a glowing square on the side.
 * @author Sterling Hirsh
 * @date 2/24/2011
 */

#include "Items/GlowSquare.h"
#include <algorithm>
#include <math.h>

#ifdef WIN32
#include "Utility/WindowsMathLib.h"
#endif

GlowSquare::GlowSquare(Color* _color,
 float size, float _x, float _y, float _z, BoundingWall* _wall,
 int _xIndex, int _yIndex) : 
 color(_color), wall(_wall), x(_xIndex), y(_yIndex) {
   timeLastHit = 0;
   if (wall->wallID % 3 == 0) {
      // Top or bottom
      p1.update(_x, _y, _z);
      p2.update(_x + size, _y, _z);
      p3.update(_x + size, _y, _z + size);
      p4.update(_x, _y, _z + size);
   } else if (wall->wallID % 3 == 1) {
      // Front or back
      p1.update(_x, _y, _z);
      p2.update(_x + size, _y, _z);
      p3.update(_x + size, _y + size, _z);
      p4.update(_x, _y + size, _z);
   } else {
      // Left or right
      p1.update(_x, _y, _z);
      p2.update(_x, _y + size, _z);
      p3.update(_x, _y + size, _z + size);
      p4.update(_x, _y, _z + size);
   }

   switch(wall->wallID) {
      case WALL_TOP: normal.updateMagnitude(0, -1, 0); break;
      case WALL_BOTTOM: normal.updateMagnitude(0, 1, 0); break;
      case WALL_FRONT: normal.updateMagnitude(0, 0, -1); break;
      case WALL_BACK: normal.updateMagnitude(0, 0, 1); break;
      case WALL_LEFT: normal.updateMagnitude(1, 0, 0); break;
      case WALL_RIGHT: normal.updateMagnitude(-1, 0, 0); break;
   }

   midpoint1.midpoint(p1, p2);
   midpoint2.midpoint(p2, p3);
   midpoint3.midpoint(p3, p4);
   midpoint4.midpoint(p4, p1);

   initDisplayList();
}

GlowSquare::~GlowSquare() {
   // Don't delete color, since it's a shared pointer.
}

void GlowSquare::drawLines() {
   /*
   // This draws the empty box. Let's do something more interesting.
   glPolygonMode( GL_FRONT_AND_BACK, GL_LINE );
   glBegin(GL_QUADS);
   p1.draw();
   p2.draw();
   p3.draw();
   p4.draw();
   glEnd();
   */

   /*
   p1.draw();
   midpoint1.draw();
   p3.draw();
   midpoint2.draw();
   */
   /*
   midpoint1.draw();
   midpoint2.draw();
   midpoint3.draw();
   midpoint4.draw();
   */
   midpoint1.draw();
   p1.draw();
   midpoint4.draw();
}

void GlowSquare::drawGlowingPart() {
   glBegin(GL_QUADS);
   /*
   midpoint1.draw();
   midpoint2.draw();
   midpoint3.draw();
   midpoint4.draw();
   */
   /*
   p1.draw();
   p2.draw();
   p3.draw();
   p4.draw();
   */

   p1.draw();
   midpoint1.draw();
   p3.draw();
   midpoint3.draw();

   glEnd();
}

void GlowSquare::draw() {
   double timeDiff;
   const double fadeTime = 0.75;
   const double shwobbleAmplitude = 0.5;
   const int numShwobbles = 1;
   const double fadeScale = 1 / fadeTime;
   timeDiff = (doubleTime() - timeLastHit) / fadeTime;
   
   // Set the timeLastHit to the latest time that is <= the current time.
   while (flashTimes.size() > 0 && flashTimes.top() <= doubleTime()) {
      timeLastHit = flashTimes.top();
      flashTimes.pop();
      timeDiff = (doubleTime() - timeLastHit) / fadeTime;
   }


   // This draws the glowing filled in square.
   if (timeDiff < fadeTime) {
      glPushMatrix();
      // If the timeLastHit is after the current time, consider it not hit yet.
      alpha = (float) (fadeScale * (fadeTime - clamp(timeDiff, 0, fadeTime)));
      //glPolygonMode( GL_FRONT_AND_BACK, GL_FILL );
      // Max alpha is 0.5
      color->setColorWithAlpha((float) (alpha * 0.5));
      normal.glTranslate(shwobbleAmplitude * sin(numShwobbles * 2 * M_PI * alpha * alpha));
      glCallList(displayList);
      glPopMatrix();
   }

}

void GlowSquare::drawGlow() {
   draw();
}

void GlowSquare::update(double timeDiff) {
}

/**
 * This is what happens when you something bounces off a glow square.
 * Delay is added to the timeLastHit so that the ripple effect works.
 */
void GlowSquare::hit(int distanceLimit, double delay) {
   timeLastHit = doubleTime();

   // Now trigger neighbors
   std::vector<GlowSquare*>::iterator iter = wall->squares.begin();

   delay = clamp(delay, 0.1, 0.2);

   int distance;
   //const double delay = 0.1;
   int xDist, yDist;
   for (; iter != wall->squares.end(); ++iter) {
      xDist = abs((*iter)->x - x);
      yDist = abs((*iter)->y - y);
      // Weird
      //distance = sqrt(xDist * xDist * xDist + yDist * yDist * yDist);
      // Circle
      distance = (int) sqrt((double)(xDist * xDist + yDist * yDist));
      // Box
      //distance = std::max(xDist, yDist);
      // Diamond
      // distance = xDist + yDist;
      if (distance <= distanceLimit) {
         (*iter)->flashTimes.push(timeLastHit + (distance * delay));
      }
   }

}

void GlowSquare::initDisplayList() {
   displayList = glGenLists(1);
   glNewList(displayList, GL_COMPILE);
   drawGlowingPart();
   glEndList();
}
