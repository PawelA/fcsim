/*
* Copyright (c) 2006-2007 Erin Catto http://www.gphysics.com
*
* This software is provided 'as-is', without any express or implied
* warranty.  In no event will the authors be held liable for any damages
* arising from the use of this software.
* Permission is granted to anyone to use this software for any purpose,
* including commercial applications, and to alter it and redistribute it
* freely, subject to the following restrictions:
* 1. The origin of this software must not be misrepresented; you must not
* claim that you wrote the original software. If you use this software
* in a product, an acknowledgment in the product documentation would be
* appreciated but is not required.
* 2. Altered source versions must be plainly marked as such, and must not be
* misrepresented as being the original software.
* 3. This notice may not be removed or altered from any source distribution.
*/

#ifndef B2_SETTINGS_H
#define B2_SETTINGS_H

#include <cassert>

#define NOT_USED(x) x
#define b2Assert(A) assert((A))

typedef signed char	int8;
typedef signed short int16;
typedef signed int int32;
typedef unsigned char uint8;
typedef unsigned short uint16;
typedef unsigned int uint32;
typedef float float32;

const float32 b2_pi = 3.14159265359f;

// Define your unit system here. The default system is
// meters-kilograms-seconds. For the tuning to work well,
// your dynamic objects should be bigger than a pebble and smaller
// than a house.
const float32 b2_lengthUnitsPerMeter = 30.0f;
const float32 b2_massUnitsPerKilogram = 1.0f;
const float32 b2_timeUnitsPerSecond = 1.0f;

// Use this for pixels:
//const float32 b2_lengthUnitsPerMeter = 50.0f;



// Global tuning constants based on MKS units.

// Collision
const int32 b2_maxManifoldPoints = 2;
const int32 b2_maxShapesPerBody = 64;
const int32 b2_maxPolyVertices = 8;
const int32 b2_maxProxies = 4096;				// this must be a power of two
const int32 b2_maxPairs = 32768;	// this must be a power of two

// Dynamics
const float32 b2_linearSlop = 0.15f;
const float32 b2_angularSlop = 0.03490658503988659f;
const float32 b2_velocityThreshold = 30.0f;
const float32 b2_maxLinearCorrection = 6.0f;
const float32 b2_maxAngularCorrection = 0.13962634015954636f;
const float32 b2_contactBaumgarte = 0.2f;

// Sleep
const float32 b2_timeToSleep = 0.5f;
const float32 b2_linearSleepTolerance = 0.3f;
const float32 b2_angularSleepTolerance = 0.011111111111111112f;


// Memory Allocation
extern int32 b2_byteCount;
void* b2Alloc(int32 size);
void b2Free(void* mem);

#endif
