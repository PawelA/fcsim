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

#ifndef CONTACT_SOLVER_H
#define CONTACT_SOLVER_H

#include <box2d/b2Math.h>
#include <box2d/b2Collision.h>

class b2Contact;
struct b2Body;
class b2Island;
class b2StackAllocator;

struct b2ContactConstraintPoint
{
	b2Vec2 localAnchor1;
	b2Vec2 localAnchor2;
	float64 normalImpulse;
	float64 tangentImpulse;
	float64 positionImpulse;
	float64 normalMass;
	float64 tangentMass;
	float64 separation;
	float64 velocityBias;
};

struct b2ContactConstraint
{
	b2ContactConstraintPoint points[b2_maxManifoldPoints];
	b2Vec2 normal;
	b2Manifold* manifold;
	b2Body* body1;
	b2Body* body2;
	float64 friction;
	float64 restitution;
	int32 pointCount;
};

class b2ContactSolver
{
public:
	b2ContactSolver(b2Contact** contacts, int32 contactCount, b2StackAllocator* allocator);
	~b2ContactSolver();

	void PreSolve();
	void SolveVelocityConstraints();
	bool SolvePositionConstraints(float64 beta);
	void PostSolve();

	b2StackAllocator* m_allocator;
	b2ContactConstraint* m_constraints;
	int m_constraintCount;
};

#endif
