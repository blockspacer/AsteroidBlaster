/**
 * Particles that come off the blaster shots.
 * @author Sterling Hirsh
 * @date 2/17/11
 */

#include "Particles/BlasterShotParticle.h"
#include "math.h"

BlasterShotParticle::BlasterShotParticle(Point3D* _position,
      Vector3D* _velocity, float _life, float _r, float _g, float _b) :
   Particle(_position, _velocity, _life, _r, _g, _b) {
      size = 0.1;
   }

bool BlasterShotParticle::step(double timeDifference) {
   const float velocityScalePerSecond = 3;
   velocity->xMag -= velocity->xMag * velocityScalePerSecond * timeDifference;
   velocity->yMag -= velocity->yMag * velocityScalePerSecond * timeDifference;
   velocity->zMag -= velocity->zMag * velocityScalePerSecond * timeDifference;
   return Particle::step(timeDifference);
}

void BlasterShotParticle::Add(Point3D* pos, Vector3D* vec) {
   if (particles.size() >= MAX_PARTICLES) {
      std::cout << "max particles reached!" << std::endl;
      delete pos;
      delete vec;
      return;
   }
   const float minLife = 2; // Seconds

   float _fade = (minLife * randdouble()) + minLife;
   float _r = 0.9 + (randdouble() * 0.1);
   float _g = 0;
   float _b = 0;

   particles.push_back(new BlasterShotParticle(pos, vec, _fade, _r, _g, _b));
}

void BlasterShotParticle::AddRainbow(Point3D* pos, Vector3D* vec, int particleNum, int particleCycle) {
   if (particles.size() >= MAX_PARTICLES) {
      std::cout << "max particles reached!" << std::endl;
      delete pos;
      delete vec;
      return;
   }

   const float minLife = 2; // Seconds

   float _fade = (minLife * randdouble()) + minLife;
   float _r = 0.9 + (randdouble() * 0.1) ;
   float _g = 0;
   float _b = 0;
   getBrightColor((float)particleNum / particleCycle, _r, _g, _b);

   particles.push_back(new BlasterShotParticle(pos, vec, _fade, _r, _g, _b));

}
