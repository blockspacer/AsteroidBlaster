/**
 * Sterling Hirsh
 * GlobalUtility.h
 * A collection of functions that make working with glut possible.
 * Note that global variables GW, GH, mouseX, and mouseY are defined here.
 */
#ifndef __GLOBALUTILITY_H__
#define __GLOBALUTILITY_H__

#include <stdlib.h>
#include <stdio.h>
#include <time.h>
#include <list>

#ifdef __APPLE__
#include <OpenGL/gl.h>
#include <OpenGL/glu.h>
#include <OpenGL/glext.h>
#else
#include <GL/glew.h>
#endif

#include "SDL.h"
#include "SDL_ttf.h"
#include "Utility/Constants.h"
#include "Utility/InputManager.h"
#include "Menus/MainMenu.h"
#include "Menus/StoreMenu.h"
#include "Menus/SettingsMenu.h"
#include "Menus/HelpMenu.h"
#include "Menus/CreditsMenu.h"
#include "Graphics/Texture.h"

#include "Utility/GameSettings.h"



class GameState;

extern GameSettings* gameSettings;
extern int shipId;
extern int flopY(int);
extern int texSize;
extern unsigned long curFrame;
extern bool drawPerspective;
extern bool showBloomScreen;
extern bool cameraFollow;
extern GLUquadricObj *quadric;
extern GLuint tractorBeamShader;
extern GLuint fadeShader;
extern GLuint elecShader;
extern GLuint lawnShader;
extern GLuint hBlurShader;
extern GLuint vBlurShader;
extern GLuint ramShader;
extern GLuint tractorFade;
extern GLuint billboardShader;
extern GLuint shipXShader;
extern GLuint shipYShader;
extern GLuint backShader;
extern GLuint hitShader;
extern GLuint fbo;
extern GLuint depthbuffer;
extern SDL_Surface* gDrawSurface;
extern const SDL_VideoInfo* vidinfo;
extern InputManager* inputManager;
extern MainMenu* mainMenu;
extern StoreMenu* storeMenu;
extern HelpMenu* helpMenu;
extern SettingsMenu* settingsMenu;
extern CreditsMenu* creditsMenu;
extern TTF_Font* hudFont;
extern TTF_Font* menuFont;


double p2wx(int);
double p2wy(int);
double p2wHeight(int);
double p2wWidth(int);
double p2ix(int);
double p2iy(int);
void reshape(GLsizei w, GLsizei h);
double distance3D(double x1, double y1, double z1, double x2 = 0,
 double y2 = 0, double z2 = 0);
double distance2D(double x1, double y1, double x2 = 0, double y2 = 0);
double clamp(double, double, double);
double randdouble();
GLuint setShaders(char * vert, char * frag, char * geom);
GLuint setShaders(char * vert, char * frag);
GLuint setShaders(char * vert);
char *textFileRead(char *fn);
void printShaderLog(GLuint obj);
void printProgramLog(GLuint obj);
double doubleTime();
int minutesRemaining(double secondsRemaining);
int secondsRemainder(double secondsRemaining);
void useOrtho();
void usePerspective();
void drawCylinder(double radius, double length);
void getBrightColor(double hue, float& r, float& g, float& b);
void setupVideo();
void drawGameState(GameState* gameState,
      bool lookAt = false, float eX = 0.0, float eY = 0.0, float eZ = 0.0,
      float lX = 0.0, float lY = 0.0, float lZ = 0.0,
      float uX = 0.0, float uY = 0.0, float uZ = 0.0);
void initFbo();
int nextPowerOfTwo(int num);

// Used for defining a material
typedef struct materialStruct {
  GLfloat ambient[4];
  GLfloat diffuse[4];
  GLfloat specular[4];
  GLfloat shininess[1];
  GLfloat emissive[4];
} materialStruct;


extern materialStruct GreenShiny;
extern materialStruct RedShiny;
extern materialStruct GreenTransparent;
extern materialStruct BlueShiny;
extern materialStruct RedFlat;
extern materialStruct RedTransparent;
extern materialStruct YellowFlat;
extern materialStruct YellowTransparent;
extern materialStruct WhiteTransparent;
extern materialStruct GrayTransparent;
extern materialStruct WhiteSolid;
extern materialStruct BlackSolid;
extern materialStruct OrangeSolid;
extern materialStruct CyanSolid;
extern materialStruct Rock;
extern materialStruct CrystalMaterial;

void setMaterial(materialStruct material);
void updateDoubleTime();
void toggleGrabMode();
void toggleFullScreen();

extern bool running;

struct Color {
   GLfloat r, g, b, a;
   Color(GLfloat _r, GLfloat _g, GLfloat _b, GLfloat _a = 1) :
      r(_r), g(_g), b(_b), a(_a) {}
   void setColor() {
      glColor4f(r, g, b, a);
   }
   void setColorWithAlpha(GLfloat tempAlpha) {
      glColor4f(r, g, b, tempAlpha);
   }
};

struct Timer {
   double timeStarted;
   double countDownTime;
   double timePaused;
   bool isPaused;
   bool isRunning;

   Timer() : timeStarted(0), countDownTime(0), timePaused(0), isPaused(false), isRunning(false) {}
   
   inline void reset() {
      timeStarted = doubleTime();
      countDownTime = 0;
      timePaused = 0;
      isPaused = false;
      isRunning = true;
   }

   inline void setCountDown(double _countDownTime) {
      isPaused = false;
      timePaused = 0;
      timeStarted = doubleTime();
      countDownTime = _countDownTime;
      isRunning = true;
   }

   /**
    * Set the count down to its last value.
    */
   inline void restartCountDown() {
      timeStarted = doubleTime();
      isRunning = true;
   }

   inline double getTimeLeft() {
      return timeStarted + countDownTime - doubleTime();
   }

   inline double getTimeRunning() {
      return doubleTime() - timeStarted;
   }

   /**
    * Return a number representing how long the timer has
    * been counting in units of countDownTime.
    * After just starting, this will return 0.
    * After countDownTime seconds, this will return 1.
    * After 2 * countDownTime seconds, this will return 2.
    */
   inline double getAmountComplete() {
      if (countDownTime == 0)
         return 0;

      return 1 - (getTimeLeft() / countDownTime);
   }

   /**
    * The Timer will remain at its current timeleft until it is resumed again.
    * Pausing a timer when it is already paused will do nothing.
    */
   inline void pause() {
      // As long as the game's not already paused, pause it.
      if (!getIsPaused()) {
         isPaused = true;
         isRunning = false;
         timePaused = doubleTime();
      }
   }

   /**
    * Resuming a timer will only work if it is already paused.
    */
   inline void resume() {
      // As long as the game is already paused, resume it.
      if (getIsPaused()) {
         isPaused = false;
         isRunning = true;
         timeStarted += doubleTime() - timePaused;
      }
   }
   
   inline bool getIsPaused() {
      return isPaused;
   }
};

#endif
