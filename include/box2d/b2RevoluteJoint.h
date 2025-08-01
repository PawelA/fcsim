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

#ifndef B2_REVOLUTE_JOINT_H
#define B2_REVOLUTE_JOINT_H

#include <box2d/b2Vec.h>
#include <stddef.h>

typedef struct b2Body b2Body;
struct b2Body;

typedef struct b2RevoluteJoint b2RevoluteJoint;
struct b2RevoluteJoint;

typedef struct b2TimeStep b2TimeStep;
struct b2TimeStep;

typedef struct b2RevoluteJointDef b2RevoluteJointDef;
struct b2RevoluteJointDef
{
	void* userData;
	b2Body* body1;
	b2Body* body2;
	bool collideConnected;

	b2Vec2 anchorPoint;
	float64 motorTorque;
	float64 motorSpeed;
	bool enableMotor;
};

typedef struct b2JointNode b2JointNode;
struct b2JointNode
{
	b2Body* other;
	b2RevoluteJoint* joint;
	b2JointNode* prev;
	b2JointNode* next;
};
typedef struct b2RevoluteJoint b2RevoluteJoint;
struct b2RevoluteJoint
{
	b2RevoluteJoint* m_prev;
	b2RevoluteJoint* m_next;
	b2JointNode m_node1;
	b2JointNode m_node2;
	b2Body* m_body1;
	b2Body* m_body2;

	bool m_islandFlag;
	bool m_collideConnected;

	void* m_userData;

	b2Vec2 m_localAnchor1;
	b2Vec2 m_localAnchor2;
	b2Vec2 m_ptpImpulse;
	float64 m_motorImpulse;

	b2Mat22 m_ptpMass;		// effective mass for point-to-point constraint.
	float64 m_motorMass;	// effective mass for motor/limit angular constraint.
	float64 m_maxMotorTorque;
	float64 m_motorSpeed;

	bool m_enableMotor;
};

void b2RevoluteJointDef_ctor(b2RevoluteJointDef *rev_joint_def);

b2Vec2 b2RevoluteJoint_GetAnchor1(b2RevoluteJoint *joint);
b2Vec2 b2RevoluteJoint_GetAnchor2(b2RevoluteJoint *joint);

void b2RevoluteJoint_PrepareVelocitySolver(b2RevoluteJoint *joint);
void b2RevoluteJoint_SolveVelocityConstraints(b2RevoluteJoint *joint, const b2TimeStep* step);

bool b2RevoluteJoint_SolvePositionConstraints(b2RevoluteJoint *joint);

void b2RevoluteJoint_ctor(b2RevoluteJoint *joint, const b2RevoluteJointDef* def);

#endif
