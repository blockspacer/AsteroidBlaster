
#ifndef __MENU_H__
#define __MENU_H__


#include "Utility/InputManager.h"

class Text;

class Menu : public InputReceiver {
   public:
   Menu();
   virtual ~Menu();

   virtual void update(double timeDiff);

   virtual void draw() = 0;
   
   virtual void keyUp(int key) = 0;
   virtual void keyDown(int key, int unicode) = 0;
   virtual void mouseDown(int button) = 0;
   virtual void mouseMove(int dx, int dy, int x, int y) = 0;
   virtual void mouseUp(int button) = 0;
   
   //the current mouse location
   double x, y;
   
   bool menuActive;
   
   
   std::vector<Text*> menuTexts;
   
   
};


#endif

