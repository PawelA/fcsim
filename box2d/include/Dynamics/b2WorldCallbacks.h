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

#ifndef B2_WORLD_CALLBACKS_H
#define B2_WORLD_CALLBACKS_H

#include "../Common/b2Settings.h"

struct b2Body;
class b2Joint;
class b2Shape;

// Implement this class to provide collision filtering. In other words, you can implement
// this class if you want finer control over contact creation.
class b2CollisionFilter
{
public:
	virtual ~b2CollisionFilter() {}

	// Return true if contact calculations should be performed between these two shapes.
	virtual bool ShouldCollide(b2Shape* shape1, b2Shape* shape2);
};

extern b2CollisionFilter b2_defaultFilter;
#endif
