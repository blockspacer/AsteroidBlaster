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
#include <GLUT/glut.h>
#include <OpenGL/glext.h>
#else
#include <GL/glew.h>
#define GLUT_DISABLE_ATEXIT_HACK 
#include <GL/glut.h>
#endif

#include "SDL.h"
#define VERT_FOV 50.0

//Global Variables declared
extern int GW, GH;
extern int flopY(int);
extern unsigned long curFrame;
extern bool drawPerspective;
extern GLUquadricObj *quadric;
extern GLuint shader1;
extern SDL_Surface* gDrawSurface;

double p2wx(int);
double p2wy(int);
double p2ix(int);
double p2iy(int);
void reshape(GLsizei w, GLsizei h);
double distance3D(double x1, double y1, double z1, double x2 = 0,
 double y2 = 0, double z2 = 0);
double distance2D(double x1, double y1, double x2 = 0, double y2 = 0);
double clamp(double, double, double);
double randdouble();
GLuint setShaders(char * vert, char * frag, char * geom);
char *textFileRead(char *fn);
void printShaderLog(GLuint obj);
void printProgramLog(GLuint obj);
double doubleTime();
void useOrtho();
void usePerspective();
void drawCylinder(double radius, double length);

// Used for defining a material
typedef struct materialStruct {
  GLfloat ambient[4];
  GLfloat diffuse[4];
  GLfloat specular[4];
  GLfloat shininess[1];
} materialStruct;

extern materialStruct GreenShiny;
extern materialStruct GreenTransparent;
extern materialStruct BlueShiny;
extern materialStruct RedFlat;
extern materialStruct RedTransparent;
extern materialStruct YellowFlat;
extern materialStruct YellowTransparent;
extern materialStruct WhiteTransparent;
extern materialStruct WhiteSolid;
extern materialStruct BlackSolid;
extern materialStruct OrangeSolid;
extern materialStruct CyanSolid;
extern materialStruct Rock;

void setMaterial(materialStruct material);


#endif