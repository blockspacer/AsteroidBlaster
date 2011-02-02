/**
 * @file
 * The AI interface.
 * <pre>
 * This header file contains the interface that every AI component must
 * implement. All AI must have a think function.
 * </pre>
 * @author Mike Smith
 */

#ifndef _AI_H_
#define _AI_H_

class AI {
   public:
      virtual int think(double dt)=0;
};

#endif