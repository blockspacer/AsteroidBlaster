
#include "Utility/Texture.h"
#include <iostream>
#include <map>
#include <math.h>
#include <GL/gl.h>
#include <GL/glu.h>
#include "SDL_image.h"

std::map<std::string, unsigned int> Texture::textures;
int Texture::numOfTextures = 0;


Texture::Texture() {

}

Texture::~Texture() {
   //std::map<std::string, unsigned int>::iterator it = Texture::textures.begin();
   //for (; it != Texture::textures.end(); ++it){
   //   SDL_FreeSurface(it->second);
   //}
}

void Texture::Add(std::string file, std::string keyName) {
   std::map<std::string, unsigned int>::iterator iter = textures.find(keyName);

   if (iter != textures.end()) {
      std::cerr << "That texture keyName (" << keyName << ") is already in use!" << std::endl;
      exit(1);
   }


   SDL_Surface *image, *temp;
   int x = 0, y = 0, w, h;
   SDL_Rect area;
   unsigned int texture;

   image=IMG_Load(file.c_str());
   if(!image) {
      printf("IMG_Load: %s\n", IMG_GetError());
      SDL_Quit();
      exit(1);
   }

   // set color key
   SDL_SetColorKey(image, SDL_SRCCOLORKEY | SDL_RLEACCEL, SDL_MapRGB(image->format, 255, 0, 128));

   // set width and height
   int width = image->w;
   int height = image->h;
   
   // get the closest w and h that's in the power of 2
   int tempLoop = 1;
   int loopVal = 1;
   while (loopVal < width) {
      loopVal = pow(tempLoop, 2);
      tempLoop++;
   }
   w = loopVal;
   
   tempLoop = 1;
   loopVal = 1;
   while (loopVal < height) {
      loopVal = pow(tempLoop, 2);
      tempLoop++;
   }
   h = loopVal;

   


   /* Create a 32-bit surface with the bytes of each pixel in R,G,B,A order,       
   as expected by OpenGL for textures */    
   Uint32 rmask, gmask, bmask, amask;    
   /* SDL interprets each pixel as a 32-bit number, so our masks must depend       
   on the endianness (byte order) of the machine */
#if SDL_BYTEORDER == SDL_BIG_ENDIAN    
   rmask = 0xff000000;
   gmask = 0x00ff0000;
   bmask = 0x0000ff00;
   amask = 0x000000ff;
#else    
   rmask = 0x000000ff;  
   gmask = 0x0000ff00;
   bmask = 0x00ff0000;
   amask = 0xff000000;
#endif

   //SDL_SetAlpha( image, SDL_SRCALPHA | SDL_RLEACCEL, amask ); 
   // blit from bitmap to a temporary surface with 32 bits per pixel
   temp = SDL_CreateRGBSurface(SDL_SWSURFACE, w, h, 32, rmask, gmask, bmask, amask);

   if(temp == NULL){
      std::cerr << "Failed to create temporary surface..." << std::endl;
      exit (1);
   }

   // set up rectangle
   area.x = x * width;
   area.y = y * height;
   area.w = width;
   area.h = height;

   if(SDL_BlitSurface(image, &area, temp, NULL)== -1){
      std::cerr << "Failed to blit bitmap to temporary surface..." << std::endl;
      exit (1);
   }

   GLenum texture_format;
   GLint  nOfColors;
   // get the number of channels in the SDL surface
   nOfColors = temp->format->BytesPerPixel;
   if (nOfColors == 4)     // contains an alpha channel
   {
      if (temp->format->Rmask == 0x000000ff)
         texture_format = GL_RGBA;
      else
         texture_format = GL_BGRA;
      } else if (nOfColors == 3)     // no alpha channel
   {
      if (temp->format->Rmask == 0x000000ff)
      texture_format = GL_RGB;
      else
      texture_format = GL_BGR;
      } else {
      printf("warning: the image is not truecolor..  this will probably break\n");
      // this error should not go unhandled
   }

   // create texture
   glGenTextures(1, &texture );
   glBindTexture(GL_TEXTURE_2D, texture );
   
   if(temp->pixels == NULL){
      std::cerr << "Failed to create texture..." << std::endl;
      exit (1);
   }

   // if temporary surface must be locked
   // lock temporary surface
   if(SDL_MUSTLOCK(temp)) {
      SDL_LockSurface(temp);
   }

   // turn bitmap into texture
   glTexImage2D(GL_TEXTURE_2D, 0, nOfColors, temp->w, temp->h, 0, texture_format, GL_UNSIGNED_BYTE, temp->pixels );
   
   // if temporary surface had to be locked
   // unlock temporary surface again
   if(SDL_MUSTLOCK(temp)) {
      SDL_UnlockSurface(temp );
   }

   // set texture filtering
   glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
   glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);

   // free temporary surface
   SDL_FreeSurface(temp);

   // free image
   SDL_FreeSurface(image);


   textures.insert(std::pair<std::string, unsigned int>(keyName,texture));
     
   numOfTextures++;
}


unsigned int Texture::getTexture(std::string keyName) {
   std::map<std::string, unsigned int>::iterator iter = textures.find(keyName);

   if (iter == textures.end()) {
      std::cerr << "failed to find: " << keyName << " in Texture::textures." << std::endl;
      exit(1);
   }
   
   return iter->second;
}



