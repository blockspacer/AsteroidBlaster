/**
 * Custodian - Stores where all the stuff is.
 * @author Sterling Hirsh
 * @date 1-26-11
 * CPE 476
 */

#include "Utility/Custodian.h"
#include <algorithm>

/**
 * Compares by minX in Object3D.
 * Returns true if first argument goes before the second argument.* Sorts nulls last.
 */
static bool compareByMinX (Object3D* item1, Object3D* item2) {
   // If the first item is null, the second item comes first.
   if (item1 == NULL)
      return false;
   // If the second item is null, the first item comes first.
   if (item2 == NULL)
      return true;
   return item1->minPosition->x < item2->minPosition->x;
}

/**
 * Compares by maxX in Object3D.
 * Returns true if first argument goes before the second argument.* Sorts nulls last.
 */
static bool compareByMaxX (Object3D* item1, Object3D* item2) {
   // If the first item is null, the second item comes first.
   if (item1 == NULL)
      return false;
   // If the second item is null, the first item comes first.
   if (item2 == NULL)
      return true;
   return item1->maxPosition->x < item2->maxPosition->x;
}

/**
 * Run this after all objects have moved.
 * This sorts all the lists, then calculates each item's rank.
 * All NULL items are pushed to the end of the lists, where 
 * they are removed from the list.
 */
void Custodian::update() {
   int firstNull = objectsByMinX.size();
   // Sort. Duh.
   sort(objectsByMinX.begin(), objectsByMinX.end(), compareByMinX);
   sort(objectsByMaxX.begin(), objectsByMaxX.end(), compareByMaxX);

   // Set up ranks in objects.
   for (int i = 0; i < objectsByMinX.size(); ++i) {
      if (objectsByMinX[i] == NULL) {
         firstNull = i;
         break;
      }
      objectsByMinX[i]->minXRank = i;
      objectsByMaxX[i]->maxXRank = i;
   }

   // Remove null pointers from the ends.
   for (int i = firstNull; i < objectsByMinX.size(); ++i) {
      objectsByMinX.pop_back();
      objectsByMaxX.pop_back();
   }
}

/**
 * Call this on an object if you want it to be custodian'd.
 */
void Custodian::add(Object3D* objectIn) {
   objectsByMinX.push_back(objectIn);
   objectsByMaxX.push_back(objectIn);
}

/**
 * Remove an object.
 */
void Custodian::remove(Object3D* objectIn) {
   objectsByMinX[objectIn->minXRank] = NULL;
   objectsByMaxX[objectIn->maxXRank] = NULL;
   delete objectIn;
}

/**
 * This returns a pointer to a new std::list. You must delete
 * if after you use it.
 */
std::set<Object3D*>* Custodian::findCollisions(Object3D* item, bool searchBackwards) {
   std::set<Object3D*>* sublist = new std::set<Object3D*>();
   std::set<Object3D*>::iterator iter;
   int numElements = objectsByMinX.size();
   Object3D* other;

   if (searchBackwards) {
      /* Start at the current maxX - 1. Check whether elements to 
       * the left have a maxX that is above item's minX.
       */
      for (int i = item->maxXRank - 1; i > 0; --i) {
         other = objectsByMaxX[i];
         if (other == NULL)
            continue;
         if (other->maxPosition->x > item->minPosition->x)
            sublist->insert(other);
         else break;
      }
   }
   
   /* Start at the current minX + 1. Check whether elements to
    * the right have a minX that is below item's maxX.
    */
   for (int i = item->maxXRank + 1; i < numElements; ++i) {
      other = objectsByMaxX[i];
      if (other == NULL)
         continue;
      if (other->minPosition->x < item->maxPosition->x)
         sublist->insert(other);
      else break;
   }

   /* Now check the y direction. From now on, we only 
    * remove elements.
    */
   for (iter = sublist->begin(); iter != sublist->end(); ++iter) {
      // If items definitely don't intersect.
      if (other->maxPosition->y < item->minPosition->y || other->minPosition->y > item->maxPosition->y ||
       other->maxPosition->z < item->minPosition->z || other->minPosition->z > item->maxPosition->z) {
         sublist->erase(iter);
      }
   }
   return sublist;
}

std::vector<Object3D*>* Custodian::getListOfObjects() {
   return &objectsByMinX;
}

void Custodian::clear() {
   objectsByMinX.clear();
   objectsByMaxX.clear();
}