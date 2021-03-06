/**
 * Text class
 * Stores everything you need to draw text on screen. Uses GlobalUtility just for GW/GH info so very easy to export
 * to another project. It can handle simple string, string-int-string, string-string-string and string-double-string.
 * @author Ryuho Kudo <ryuhokudo@gmail.com>
 * @date 3/11/11
 */

#ifndef __TEXT_H__
#define __TEXT_H__

#define DEFAULT_FONT "Fonts/Slider.ttf"

#include <string>
#include <sstream>
#include "SDL.h"
#include <SDL_ttf.h>
#include "Utility/GlobalUtility.h"

static const SDL_Color SDL_WHITE = {255,255,255};
static const SDL_Color SDL_GREY = {128,128,128};
static const SDL_Color SDL_BLACK = {0,0,0};
static const SDL_Color SDL_RED = {255,0,0}; 
static const SDL_Color SDL_GREEN = {0,255,0}; 
static const SDL_Color SDL_BLUE = {0,0,255}; 
static const SDL_Color SDL_YELLOW = {255,255,0}; 

class Text {
   private:
      void SDL_GL_RenderText(const char *text, 
                      TTF_Font *font,
                      SDL_Color color,
                      SDL_Rect *location);
                      
      void glEnable2D();
      void glDisable2D();
   public:
      //Constructor if you are displaying one string
      Text(std::string text, TTF_Font* _font, SDL_Rect _pos);
      //Text(std::string text, std::string fontName, SDL_Rect _pos, int _size, bool _centered);
      // Constructor if you are displaying multiple strings
      Text(std::string preText, std::string body, std::string postText, TTF_Font* _font, SDL_Rect _pos);
      // Constructor if you are displaying an int
      Text(std::string preText, int body, std::string postText, TTF_Font* _font, SDL_Rect _pos);
      // Constructor if you are displaying a double
      Text(std::string preText, double body, std::string postText, TTF_Font* _font, SDL_Rect _pos);

      // Destructor
      virtual ~Text();

      // Sets the new body text to be used
      void updateBody(std::string newText);
      // Sets the new body text to be used
      void updateBody(int newInt);
      // Sets the new body text to be used
      void updateBody(double newDouble);
      // Sets the font to be used
      void setFont(TTF_Font* _font);
      // Sets the color of the text
      void setColor(SDL_Color _color);
      // Sets the position where the text will be drawn
      void setPosition(SDL_Rect val);
      // Gets the position of the text, and puts it into x and y
      SDL_Rect getPosition();
      // Takes in the x and y position of where to draw the text
      void draw();
      // takes in x,y and answers if the mouse is over the text
      bool mouseSelect(int x, int y);
      bool mouseSelect(double x, double y);
      // takes in x,y and highlights the text in a color if the mouse is over the text
      void mouseHighlight(int x, int y);
      void mouseHighlight(double x, double y);
      
      TTF_Font* font;
      SDL_Color color;
      SDL_Surface* textSurface;
      SDL_Rect pos;
      int size;
      std::ostringstream sstream;
      std::string textToDisplay, pre, post;
      GLuint textTexture;
      bool selectable, selected, disabled;
      //bool centered;
      int alignment;
};

#endif
