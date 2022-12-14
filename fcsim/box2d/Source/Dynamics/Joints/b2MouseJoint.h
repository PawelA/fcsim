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

#ifndef B2_MOUSE_JOINT_H
#define B2_MOUSE_JOINT_H

#include "b2Joint.h"

struct b2MouseJointDef : public b2JointDef
{
	b2MouseJointDef()
	{
		type = e_mouseJoint;
		target.Set(0.0, 0.0);
		maxForce = 0.0;
		frequencyHz = 5.0;
		dampingRatio = 0.7;
		timeStep = 1.0 / 60.0;
	}

	b2Vec2 target;
	float64 maxForce;
	float64 frequencyHz;
	float64 dampingRatio;
	float64 timeStep;
};

class b2MouseJoint : public b2Joint
{
public:
	b2Vec2 GetAnchor1() const;
	b2Vec2 GetAnchor2() const;

	b2Vec2 GetReactionForce(float64 invTimeStep) const;
	float64 GetReactionTorque(float64 invTimeStep) const;

	void SetTarget(const b2Vec2& target);

	//--------------- Internals Below -------------------

	b2MouseJoint(const b2MouseJointDef* def);

	void PrepareVelocitySolver();
	void SolveVelocityConstraints(const b2TimeStep* step);
	bool SolvePositionConstraints()
	{
		return true;
	}

	b2Vec2 m_localAnchor;
	b2Vec2 m_target;
	b2Vec2 m_impulse;

	b2Mat22 m_ptpMass;		// effective mass for point-to-point constraint.
	b2Vec2 m_C;				// position error
	float64 m_maxForce;
	float64 m_beta;			// bias factor
	float64 m_gamma;		// softness
};

#endif
