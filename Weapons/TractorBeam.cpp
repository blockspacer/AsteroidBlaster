/**
 * Tractor Beam
 * This pulls in crystals.
 * @author Sterling Hirsh (shirsh@calpoly.edu)
 * @date Valentine's Day <3
 */

#include "Weapons/TractorBeam.h"
#include "Utility/GlobalUtility.h"
#include "Shots/TractorBeamShot.h"

TractorBeam::TractorBeam(AsteroidShip* owner) : Weapon(owner) {
   coolDown = 0;
   name = "Tractor Beam";
}

TractorBeam::~TractorBeam() {
   // Do nothing.
}

void TractorBeam::update(double timeDiff) {
   // Do nothing.
}

void TractorBeam::fire() {
   Point3D start = *ship->position;
   gameState->custodian.add(new TractorBeamShot(start, ship->shotDirection, ship));
   //std::set<Object3D*>* tempList = gameState->custodian.findCollisions(new TractorBeamShot(start, ship->shotDirection, ship));
   
}

void TractorBeam::debug() {
   printf("TractorBeam!\n");
}

Point3D TractorBeam::project(Object3D* object) {
   return Point3D::Zero;
}

