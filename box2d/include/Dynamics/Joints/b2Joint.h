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

#ifndef JOINT_H
#define JOINT_H

#include "../../Common/b2Math.h"

struct b2Body;
struct b2Joint;
struct b2TimeStep;
class b2BlockAllocator;

enum b2JointType
{
	e_unknownJoint,
	e_revoluteJoint,
	e_prismaticJoint,
	e_distanceJoint,
	e_pulleyJoint,
	e_mouseJoint,
	e_gearJoint
};

enum b2LimitState
{
	e_inactiveLimit,
	e_atLowerLimit,
	e_atUpperLimit,
	e_equalLimits
};

struct b2Jacobian
{
	b2Vec2 linear1;
	float64 angular1;
	b2Vec2 linear2;
	float64 angular2;

	void SetZero();
	void Set(const b2Vec2& x1, float64 a1, const b2Vec2& x2, float64 a2);
	float64 Compute(const b2Vec2& x1, float64 a1, const b2Vec2& x2, float64 a2);
};

struct b2JointNode
{
	b2Body* other;
	b2Joint* joint;
	b2JointNode* prev;
	b2JointNode* next;
};

struct b2JointDef
{
	b2JointDef()
	{
		type = e_unknownJoint;
		userData = NULL;
		body1 = NULL;
		body2 = NULL;
		collideConnected = false;
	}

	b2JointType type;
	void* userData;
	b2Body* body1;
	b2Body* body2;
	bool collideConnected;
};

struct b2Joint
{
	b2Vec2 (*GetAnchor1)(b2Joint *joint);
	b2Vec2 (*GetAnchor2)(b2Joint *joint);

	b2Vec2 (*GetReactionForce)(b2Joint *joint, float64 invTimeStep);
	float64 (*GetReactionTorque)(b2Joint *joint, float64 invTimeStep);

	void (*PrepareVelocitySolver)(b2Joint *joint);
	void (*SolveVelocityConstraints)(b2Joint *joint, const b2TimeStep* step);

	bool (*SolvePositionConstraints)(b2Joint *joint);

	b2JointType m_type;
	b2Joint* m_prev;
	b2Joint* m_next;
	b2JointNode m_node1;
	b2JointNode m_node2;
	b2Body* m_body1;
	b2Body* m_body2;

	bool m_islandFlag;
	bool m_collideConnected;

	void* m_userData;
};

inline void b2Jacobian::SetZero()
{
	b2Vec2_SetZero(&linear1); angular1 = 0.0;
	b2Vec2_SetZero(&linear2); angular2 = 0.0;
}

inline void b2Jacobian::Set(const b2Vec2& x1, float64 a1, const b2Vec2& x2, float64 a2)
{
	linear1 = x1; angular1 = a1;
	linear2 = x2; angular2 = a2;
}

inline float64 b2Jacobian::Compute(const b2Vec2& x1, float64 a1, const b2Vec2& x2, float64 a2)
{
	return b2Dot(linear1, x1) + angular1 * a1 + b2Dot(linear2, x2) + angular2 * a2;
}

b2Joint* b2Joint_Create(const b2JointDef* def, b2BlockAllocator* allocator);
void b2Joint_Destroy(b2Joint* joint, b2BlockAllocator* allocator);

void b2Joint_ctor(b2Joint *joint, const b2JointDef* def);

#endif
