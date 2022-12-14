/*
* Copyright (c) 2007 Erin Catto http://www.gphysics.com
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

#include "b2PulleyJoint.h"
#include "../b2Body.h"
#include "../b2World.h"

// Pulley:
// length1 = norm(p1 - s1)
// length2 = norm(p2 - s2)
// C0 = (length1 + ratio * length2)_initial
// C = C0 - (length1 + ratio * length2) = 0
// u1 = (p1 - s1) / norm(p1 - s1)
// u2 = (p2 - s2) / norm(p2 - s2)
// Cdot = -dot(u1, v1 + cross(w1, r1)) - ratio * dot(u2, v2 + cross(w2, r2))
// J = -[u1 cross(r1, u1) ratio * u2  ratio * cross(r2, u2)]
// K = J * invM * JT
//   = invMass1 + invI1 * cross(r1, u1)^2 + ratio^2 * (invMass2 + invI2 * cross(r2, u2)^2)
//
// Limit:
// C = maxLength - length
// u = (p - s) / norm(p - s)
// Cdot = -dot(u, v + cross(w, r))
// K = invMass + invI * cross(r, u)^2
// 0 <= impulse

b2PulleyJoint::b2PulleyJoint(const b2PulleyJointDef* def)
: b2Joint(def)
{
	m_ground = m_body1->m_world->m_groundBody;
	m_groundAnchor1 = def->groundPoint1 - m_ground->m_position;
	m_groundAnchor2 = def->groundPoint2 - m_ground->m_position;
	m_localAnchor1 = b2MulT(m_body1->m_R, def->anchorPoint1 - m_body1->m_position);
	m_localAnchor2 = b2MulT(m_body2->m_R, def->anchorPoint2 - m_body2->m_position);

	m_ratio = def->ratio;

	b2Vec2 d1 = def->groundPoint1 - def->anchorPoint1;
	b2Vec2 d2 = def->groundPoint2 - def->anchorPoint2;

	float64 length1 = b2Max(0.5 * b2_minPulleyLength, d1.Length());
	float64 length2 = b2Max(0.5 * b2_minPulleyLength, d2.Length());

	m_constant = length1 + m_ratio * length2;

	m_maxLength1 = b2Clamp(def->maxLength1, length1, m_constant - m_ratio * b2_minPulleyLength);
	m_maxLength2 = b2Clamp(def->maxLength2, length2, (m_constant - b2_minPulleyLength) / m_ratio);

	m_pulleyImpulse = 0.0;
	m_limitImpulse1 = 0.0;
	m_limitImpulse2 = 0.0;
}

void b2PulleyJoint::PrepareVelocitySolver()
{
	b2Body* b1 = m_body1;
	b2Body* b2 = m_body2;

	b2Vec2 r1 = b2Mul(b1->m_R, m_localAnchor1);
	b2Vec2 r2 = b2Mul(b2->m_R, m_localAnchor2);

	b2Vec2 p1 = b1->m_position + r1;
	b2Vec2 p2 = b2->m_position + r2;

	b2Vec2 s1 = m_ground->m_position + m_groundAnchor1;
	b2Vec2 s2 = m_ground->m_position + m_groundAnchor2;

	// Get the pulley axes.
	m_u1 = p1 - s1;
	m_u2 = p2 - s2;

	float64 length1 = m_u1.Length();
	float64 length2 = m_u2.Length();

	if (length1 > b2_linearSlop)
	{
		m_u1 *= 1.0 / length1;
	}
	else
	{
		m_u1.SetZero();
	}

	if (length2 > b2_linearSlop)
	{
		m_u2 *= 1.0 / length2;
	}
	else
	{
		m_u2.SetZero();
	}

	if (length1 < m_maxLength1)
	{
		m_limitState1 = e_inactiveLimit;
		m_limitImpulse1 = 0.0;
	}
	else
	{
		m_limitState1 = e_atUpperLimit;
		m_limitPositionImpulse1 = 0.0;
	}

	if (length2 < m_maxLength2)
	{
		m_limitState2 = e_inactiveLimit;
		m_limitImpulse2 = 0.0;
	}
	else
	{
		m_limitState2 = e_atUpperLimit;
		m_limitPositionImpulse2 = 0.0;
	}

	// Compute effective mass.
	float64 cr1u1 = b2Cross(r1, m_u1);
	float64 cr2u2 = b2Cross(r2, m_u2);

	m_limitMass1 = b1->m_invMass + b1->m_invI * cr1u1 * cr1u1;
	m_limitMass2 = b2->m_invMass + b2->m_invI * cr2u2 * cr2u2;
	m_pulleyMass = m_limitMass1 + m_ratio * m_ratio * m_limitMass2;
	b2Assert(m_limitMass1 > MIN_VALUE);
	b2Assert(m_limitMass2 > MIN_VALUE);
	b2Assert(m_pulleyMass > MIN_VALUE);
	m_limitMass1 = 1.0 / m_limitMass1;
	m_limitMass2 = 1.0 / m_limitMass2;
	m_pulleyMass = 1.0 / m_pulleyMass;

	// Warm starting.
	b2Vec2 P1 = (-m_pulleyImpulse - m_limitImpulse1) * m_u1;
	b2Vec2 P2 = (-m_ratio * m_pulleyImpulse - m_limitImpulse2) * m_u2;
	b1->m_linearVelocity += b1->m_invMass * P1;
	b1->m_angularVelocity += b1->m_invI * b2Cross(r1, P1);
	b2->m_linearVelocity += b2->m_invMass * P2;
	b2->m_angularVelocity += b2->m_invI * b2Cross(r2, P2);
}

void b2PulleyJoint::SolveVelocityConstraints(const b2TimeStep* step)
{
	NOT_USED(step);

	b2Body* b1 = m_body1;
	b2Body* b2 = m_body2;

	b2Vec2 r1 = b2Mul(b1->m_R, m_localAnchor1);
	b2Vec2 r2 = b2Mul(b2->m_R, m_localAnchor2);

	{
		b2Vec2 v1 = b1->m_linearVelocity + b2Cross(b1->m_angularVelocity, r1);
		b2Vec2 v2 = b2->m_linearVelocity + b2Cross(b2->m_angularVelocity, r2);

		float64 Cdot = -b2Dot(m_u1, v1) - m_ratio * b2Dot(m_u2, v2);
		float64 impulse = -m_pulleyMass * Cdot;
		m_pulleyImpulse += impulse;

		b2Vec2 P1 = -impulse * m_u1;
		b2Vec2 P2 = -m_ratio * impulse * m_u2;
		b1->m_linearVelocity += b1->m_invMass * P1;
		b1->m_angularVelocity += b1->m_invI * b2Cross(r1, P1);
		b2->m_linearVelocity += b2->m_invMass * P2;
		b2->m_angularVelocity += b2->m_invI * b2Cross(r2, P2);
	}

	if (m_limitState1 == e_atUpperLimit)
	{
		b2Vec2 v1 = b1->m_linearVelocity + b2Cross(b1->m_angularVelocity, r1);
		float64 Cdot = -b2Dot(m_u1, v1);
		float64 impulse = -m_limitMass1 * Cdot;
		float64 oldLimitImpulse = m_limitImpulse1;
		m_limitImpulse1 = b2Max(0.0, m_limitImpulse1 + impulse);
		impulse = m_limitImpulse1 - oldLimitImpulse;
		b2Vec2 P1 = -impulse * m_u1;
		b1->m_linearVelocity += b1->m_invMass * P1;
		b1->m_angularVelocity += b1->m_invI * b2Cross(r1, P1);
	}

	if (m_limitState2 == e_atUpperLimit)
	{
		b2Vec2 v2 = b2->m_linearVelocity + b2Cross(b2->m_angularVelocity, r2);
		float64 Cdot = -b2Dot(m_u2, v2);
		float64 impulse = -m_limitMass2 * Cdot;
		float64 oldLimitImpulse = m_limitImpulse2;
		m_limitImpulse2 = b2Max(0.0, m_limitImpulse2 + impulse);
		impulse = m_limitImpulse2 - oldLimitImpulse;
		b2Vec2 P2 = -impulse * m_u2;
		b2->m_linearVelocity += b2->m_invMass * P2;
		b2->m_angularVelocity += b2->m_invI * b2Cross(r2, P2);
	}
}

bool b2PulleyJoint::SolvePositionConstraints()
{
	b2Body* b1 = m_body1;
	b2Body* b2 = m_body2;

	b2Vec2 s1 = m_ground->m_position + m_groundAnchor1;
	b2Vec2 s2 = m_ground->m_position + m_groundAnchor2;

	float64 linearError = 0.0;

	{
		b2Vec2 r1 = b2Mul(b1->m_R, m_localAnchor1);
		b2Vec2 r2 = b2Mul(b2->m_R, m_localAnchor2);

		b2Vec2 p1 = b1->m_position + r1;
		b2Vec2 p2 = b2->m_position + r2;

		// Get the pulley axes.
		m_u1 = p1 - s1;
		m_u2 = p2 - s2;

		float64 length1 = m_u1.Length();
		float64 length2 = m_u2.Length();

		if (length1 > b2_linearSlop)
		{
			m_u1 *= 1.0 / length1;
		}
		else
		{
			m_u1.SetZero();
		}

		if (length2 > b2_linearSlop)
		{
			m_u2 *= 1.0 / length2;
		}
		else
		{
			m_u2.SetZero();
		}

		float64 C = m_constant - length1 - m_ratio * length2;
		linearError = b2Max(linearError, b2Abs(C));
		C = b2Clamp(C, -b2_maxLinearCorrection, b2_maxLinearCorrection);
		float64 impulse = -m_pulleyMass * C;

		b2Vec2 P1 = -impulse * m_u1;
		b2Vec2 P2 = -m_ratio * impulse * m_u2;

		b1->m_position += b1->m_invMass * P1;
		b1->m_rotation += b1->m_invI * b2Cross(r1, P1);
		b2->m_position += b2->m_invMass * P2;
		b2->m_rotation += b2->m_invI * b2Cross(r2, P2);

		b1->m_R.Set(b1->m_rotation);
		b2->m_R.Set(b2->m_rotation);
	}

	if (m_limitState1 == e_atUpperLimit)
	{
		b2Vec2 r1 = b2Mul(b1->m_R, m_localAnchor1);
		b2Vec2 p1 = b1->m_position + r1;

		m_u1 = p1 - s1;
		float64 length1 = m_u1.Length();

		if (length1 > b2_linearSlop)
		{
			m_u1 *= 1.0 / length1;
		}
		else
		{
			m_u1.SetZero();
		}

		float64 C = m_maxLength1 - length1;
		linearError = b2Max(linearError, -C);
		C = b2Clamp(C + b2_linearSlop, -b2_maxLinearCorrection, 0.0);
		float64 impulse = -m_limitMass1 * C;
		float64 oldLimitPositionImpulse = m_limitPositionImpulse1;
		m_limitPositionImpulse1 = b2Max(0.0, m_limitPositionImpulse1 + impulse);
		impulse = m_limitPositionImpulse1 - oldLimitPositionImpulse;

		b2Vec2 P1 = -impulse * m_u1;
		b1->m_position += b1->m_invMass * P1;
		b1->m_rotation += b1->m_invI * b2Cross(r1, P1);
		b1->m_R.Set(b1->m_rotation);
	}

	if (m_limitState2 == e_atUpperLimit)
	{
		b2Vec2 r2 = b2Mul(b2->m_R, m_localAnchor2);
		b2Vec2 p2 = b2->m_position + r2;

		m_u2 = p2 - s2;
		float64 length2 = m_u2.Length();

		if (length2 > b2_linearSlop)
		{
			m_u2 *= 1.0 / length2;
		}
		else
		{
			m_u2.SetZero();
		}

		float64 C = m_maxLength2 - length2;
		linearError = b2Max(linearError, -C);
		C = b2Clamp(C + b2_linearSlop, -b2_maxLinearCorrection, 0.0);
		float64 impulse = -m_limitMass2 * C;
		float64 oldLimitPositionImpulse = m_limitPositionImpulse2;
		m_limitPositionImpulse2 = b2Max(0.0, m_limitPositionImpulse2 + impulse);
		impulse = m_limitPositionImpulse2 - oldLimitPositionImpulse;

		b2Vec2 P2 = -impulse * m_u2;
		b2->m_position += b2->m_invMass * P2;
		b2->m_rotation += b2->m_invI * b2Cross(r2, P2);
		b2->m_R.Set(b2->m_rotation);
	}

	return linearError < b2_linearSlop;
}

b2Vec2 b2PulleyJoint::GetAnchor1() const
{
	return m_body1->m_position + b2Mul(m_body1->m_R, m_localAnchor1);
}

b2Vec2 b2PulleyJoint::GetAnchor2() const
{
	return m_body2->m_position + b2Mul(m_body2->m_R, m_localAnchor2);
}

b2Vec2 b2PulleyJoint::GetGroundPoint1() const
{
	return m_ground->m_position + m_groundAnchor1;
}

b2Vec2 b2PulleyJoint::GetGroundPoint2() const
{
	return m_ground->m_position + m_groundAnchor2;
}

b2Vec2 b2PulleyJoint::GetReactionForce(float64 invTimeStep) const
{
	NOT_USED(invTimeStep);
	b2Vec2 F(0.0, 0.0); // = (m_pulleyImpulse * invTimeStep) * m_u;
	return F;
}

float64 b2PulleyJoint::GetReactionTorque(float64 invTimeStep) const
{
	NOT_USED(invTimeStep);
	return 0.0;
}

float64 b2PulleyJoint::GetLength1() const
{
	b2Vec2 p = m_body1->m_position + b2Mul(m_body1->m_R, m_localAnchor1);
	b2Vec2 s = m_ground->m_position + m_groundAnchor1;
	b2Vec2 d = p - s;
	return d.Length();
}

float64 b2PulleyJoint::GetLength2() const
{
	b2Vec2 p = m_body2->m_position + b2Mul(m_body2->m_R, m_localAnchor2);
	b2Vec2 s = m_ground->m_position + m_groundAnchor2;
	b2Vec2 d = p - s;
	return d.Length();
}

float64 b2PulleyJoint::GetRatio() const
{
	return m_ratio;
}

