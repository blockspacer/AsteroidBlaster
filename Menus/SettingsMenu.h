
#ifndef __SETTINGSMENU_H__
#define __SETTINGSMENU_H__


#include "Menus/Menu.h"
#include "Utility/InputManager.h"
#include <string>

class GameState;

class SettingsMenu : public Menu {
   public:
   SettingsMenu(GameState*& _gameState);
   virtual ~SettingsMenu();
   
   GameState*& gameState;

   std::string getStatus(bool status);
   std::string getViewStatus(int status);
   void draw();
   
   void keyUp(int key);
   void keyDown(int key, int unicode);
   void mouseDown(int button);
   void mouseMove(int dx, int dy, int x, int y);
   void mouseUp(int button);
   
   void activate();
   void deactivate();
   void newGameDeactivate();
   
   //the current mouse location
   double x, y;
   
   bool menuActive;
   bool firstTime;
   
   
   std::vector<Text*> menuTexts;
   std::vector<int> types;
   
   
};


#endif

