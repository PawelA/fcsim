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

#include "b2RevoluteJoint.h"
#include "../b2Body.h"
#include "../b2World.h"

#include "../b2Island.h"

// Point-to-point constraint
// C = p2 - p1
// Cdot = v2 - v1
//      = v2 + cross(w2, r2) - v1 - cross(w1, r1)
// J = [-I -r1_skew I r2_skew ]
// Identity used:
// w k % (rx i + ry j) = w * (-ry i + rx j)

// Motor constraint
// Cdot = w2 - w1
// J = [0 0 -1 0 0 1]
// K = invI1 + invI2

b2RevoluteJoint::b2RevoluteJoint(const b2RevoluteJointDef* def)
: b2Joint(def)
{
	m_localAnchor1 = b2MulT(m_body1->m_R, def->anchorPoint - m_body1->m_position);
	m_localAnchor2 = b2MulT(m_body2->m_R, def->anchorPoint - m_body2->m_position);

	m_intialAngle = m_body2->m_rotation - m_body1->m_rotation;

	m_ptpImpulse.Set(0.0, 0.0);
	m_motorImpulse = 0.0;
	m_limitImpulse = 0.0;
	m_limitPositionImpulse = 0.0;

	m_lowerAngle = def->lowerAngle;
	m_upperAngle = def->upperAngle;
	m_maxMotorTorque = def->motorTorque;
	m_motorSpeed = def->motorSpeed;
	m_enableLimit = def->enableLimit;
	m_enableMotor = def->enableMotor;
}

void b2RevoluteJoint::PrepareVelocitySolver()
{
	b2Body* b1 = m_body1;
	b2Body* b2 = m_body2;

	// Compute the effective mass matrix.
	b2Vec2 r1 = b2Mul(b1->m_R, m_localAnchor1);
	b2Vec2 r2 = b2Mul(b2->m_R, m_localAnchor2);

	// K    = [(1/m1 + 1/m2) * eye(2) - skew(r1) * invI1 * skew(r1) - skew(r2) * invI2 * skew(r2)]
	//      = [1/m1+1/m2     0    ] + invI1 * [r1.y*r1.y -r1.x*r1.y] + invI2 * [r1.y*r1.y -r1.x*r1.y]
	//        [    0     1/m1+1/m2]           [-r1.x*r1.y r1.x*r1.x]           [-r1.x*r1.y r1.x*r1.x]
	float64 invMass1 = b1->m_invMass, invMass2 = b2->m_invMass;
	float64 invI1 = b1->m_invI, invI2 = b2->m_invI;

	b2Mat22 K1;
	K1.col1.x = invMass1 + invMass2;	K1.col2.x = 0.0;
	K1.col1.y = 0.0;					K1.col2.y = invMass1 + invMass2;

	b2Mat22 K2;
	K2.col1.x =  invI1 * r1.y * r1.y;	K2.col2.x = -invI1 * r1.x * r1.y;
	K2.col1.y = -invI1 * r1.x * r1.y;	K2.col2.y =  invI1 * r1.x * r1.x;

	b2Mat22 K3;
	K3.col1.x =  invI2 * r2.y * r2.y;	K3.col2.x = -invI2 * r2.x * r2.y;
	K3.col1.y = -invI2 * r2.x * r2.y;	K3.col2.y =  invI2 * r2.x * r2.x;

	b2Mat22 K = K1 + K2 + K3;
	m_ptpMass = K.Invert();

	m_motorMass = 1.0 / (invI1 + invI2);

	if (m_enableMotor == false)
	{
		m_motorImpulse = 0.0;
	}

	if (m_enableLimit)
	{
		float64 jointAngle = b2->m_rotation - b1->m_rotation - m_intialAngle;
		if (b2Abs(m_upperAngle - m_lowerAngle) < 2.0 * b2_angularSlop)
		{
			m_limitState = e_equalLimits;
		}
		else if (jointAngle <= m_lowerAngle)
		{
			if (m_limitState != e_atLowerLimit)
			{
				m_limitImpulse = 0.0;
			}
			m_limitState = e_atLowerLimit;
		}
		else if (jointAngle >= m_upperAngle)
		{
			if (m_limitState != e_atUpperLimit)
			{
				m_limitImpulse = 0.0;
			}
			m_limitState = e_atUpperLimit;
		}
		else
		{
			m_limitState = e_inactiveLimit;
			m_limitImpulse = 0.0;
		}
	}
	else
	{
		m_limitImpulse = 0.0;
	}

	if (b2World::s_enableWarmStarting)
	{
		b1->m_linearVelocity -= invMass1 * m_ptpImpulse;
		b1->m_angularVelocity -= invI1 * (b2Cross(r1, m_ptpImpulse) + m_motorImpulse + m_limitImpulse);

		b2->m_linearVelocity += invMass2 * m_ptpImpulse;
		b2->m_angularVelocity += invI2 * (b2Cross(r2, m_ptpImpulse) + m_motorImpulse + m_limitImpulse);
	}
	else
	{
		m_ptpImpulse.SetZero();
		m_motorImpulse = 0.0;
		m_limitImpulse = 0.0;
	}

	m_limitPositionImpulse = 0.0;
}

void b2RevoluteJoint::SolveVelocityConstraints(const b2TimeStep* step)
{
	b2Body* b1 = m_body1;
	b2Body* b2 = m_body2;

	b2Vec2 r1 = b2Mul(b1->m_R, m_localAnchor1);
	b2Vec2 r2 = b2Mul(b2->m_R, m_localAnchor2);

	// Solve point-to-point constraint
	b2Vec2 ptpCdot = b2->m_linearVelocity + b2Cross(b2->m_angularVelocity, r2) - b1->m_linearVelocity - b2Cross(b1->m_angularVelocity, r1);
	b2Vec2 ptpImpulse = -b2Mul(m_ptpMass, ptpCdot);
	m_ptpImpulse += ptpImpulse;

	b1->m_linearVelocity -= b1->m_invMass * ptpImpulse;
	b1->m_angularVelocity -= b1->m_invI * b2Cross(r1, ptpImpulse);

	b2->m_linearVelocity += b2->m_invMass * ptpImpulse;
	b2->m_angularVelocity += b2->m_invI * b2Cross(r2, ptpImpulse);

	if (m_enableMotor && m_limitState != e_equalLimits)
	{
		float64 motorCdot = b2->m_angularVelocity - b1->m_angularVelocity - m_motorSpeed;
		float64 motorImpulse = -m_motorMass * motorCdot;
		float64 oldMotorImpulse = m_motorImpulse;
		m_motorImpulse = b2Clamp(m_motorImpulse + motorImpulse, -step->dt * m_maxMotorTorque, step->dt * m_maxMotorTorque);
		motorImpulse = m_motorImpulse - oldMotorImpulse;
		b1->m_angularVelocity -= b1->m_invI * motorImpulse;
		b2->m_angularVelocity += b2->m_invI * motorImpulse;
	}

	if (m_enableLimit && m_limitState != e_inactiveLimit)
	{
		float64 limitCdot = b2->m_angularVelocity - b1->m_angularVelocity;
		float64 limitImpulse = -m_motorMass * limitCdot;

		if (m_limitState == e_equalLimits)
		{
			m_limitImpulse += limitImpulse;
		}
		else if (m_limitState == e_atLowerLimit)
		{
			float64 oldLimitImpulse = m_limitImpulse;
			m_limitImpulse = b2Max(m_limitImpulse + limitImpulse, 0.0);
			limitImpulse = m_limitImpulse - oldLimitImpulse;
		}
		else if (m_limitState == e_atUpperLimit)
		{
			float64 oldLimitImpulse = m_limitImpulse;
			m_limitImpulse = b2Min(m_limitImpulse + limitImpulse, 0.0);
			limitImpulse = m_limitImpulse - oldLimitImpulse;
		}

		b1->m_angularVelocity -= b1->m_invI * limitImpulse;
		b2->m_angularVelocity += b2->m_invI * limitImpulse;
	}
}

bool b2RevoluteJoint::SolvePositionConstraints()
{
	b2Body* b1 = m_body1;
	b2Body* b2 = m_body2;

	float64 positionError = 0.0;

	// Solve point-to-point position error.
	b2Vec2 r1 = b2Mul(b1->m_R, m_localAnchor1);
	b2Vec2 r2 = b2Mul(b2->m_R, m_localAnchor2);

	b2Vec2 p1 = b1->m_position + r1;
	b2Vec2 p2 = b2->m_position + r2;
	b2Vec2 ptpC = p2 - p1;

	positionError = ptpC.Length();

	// Prevent overly large corrections.
	//b2Vec2 dpMax(b2_maxLinearCorrection, b2_maxLinearCorrection);
	//ptpC = b2Clamp(ptpC, -dpMax, dpMax);

	float64 invMass1 = b1->m_invMass, invMass2 = b2->m_invMass;
	float64 invI1 = b1->m_invI, invI2 = b2->m_invI;

	b2Mat22 K1;
	K1.col1.x = invMass1 + invMass2;	K1.col2.x = 0.0;
	K1.col1.y = 0.0;					K1.col2.y = invMass1 + invMass2;

	b2Mat22 K2;
	K2.col1.x =  invI1 * r1.y * r1.y;	K2.col2.x = -invI1 * r1.x * r1.y;
	K2.col1.y = -invI1 * r1.x * r1.y;	K2.col2.y =  invI1 * r1.x * r1.x;

	b2Mat22 K3;
	K3.col1.x =  invI2 * r2.y * r2.y;	K3.col2.x = -invI2 * r2.x * r2.y;
	K3.col1.y = -invI2 * r2.x * r2.y;	K3.col2.y =  invI2 * r2.x * r2.x;

	b2Mat22 K = K1 + K2 + K3;
	b2Vec2 impulse = K.Solve(-ptpC);

	b1->m_position -= b1->m_invMass * impulse;
	b1->m_rotation -= b1->m_invI * b2Cross(r1, impulse);
	b1->m_R.Set(b1->m_rotation);

	b2->m_position += b2->m_invMass * impulse;
	b2->m_rotation += b2->m_invI * b2Cross(r2, impulse);
	b2->m_R.Set(b2->m_rotation);

	// Handle limits.
	float64 angularError = 0.0;

	if (m_enableLimit && m_limitState != e_inactiveLimit)
	{
		float64 angle = b2->m_rotation - b1->m_rotation - m_intialAngle;
		float64 limitImpulse = 0.0;

		if (m_limitState == e_equalLimits)
		{
			// Prevent large angular corrections
			float64 limitC = b2Clamp(angle, -b2_maxAngularCorrection, b2_maxAngularCorrection);
			limitImpulse = -m_motorMass * limitC;
			angularError = b2Abs(limitC);
		}
		else if (m_limitState == e_atLowerLimit)
		{
			float64 limitC = angle - m_lowerAngle;
			angularError = b2Max(0.0, -limitC);

			// Prevent large angular corrections and allow some slop.
			limitC = b2Clamp(limitC + b2_angularSlop, -b2_maxAngularCorrection, 0.0);
			limitImpulse = -m_motorMass * limitC;
			float64 oldLimitImpulse = m_limitPositionImpulse;
			m_limitPositionImpulse = b2Max(m_limitPositionImpulse + limitImpulse, 0.0);
			limitImpulse = m_limitPositionImpulse - oldLimitImpulse;
		}
		else if (m_limitState == e_atUpperLimit)
		{
			float64 limitC = angle - m_upperAngle;
			angularError = b2Max(0.0, limitC);

			// Prevent large angular corrections and allow some slop.
			limitC = b2Clamp(limitC - b2_angularSlop, 0.0, b2_maxAngularCorrection);
			limitImpulse = -m_motorMass * limitC;
			float64 oldLimitImpulse = m_limitPositionImpulse;
			m_limitPositionImpulse = b2Min(m_limitPositionImpulse + limitImpulse, 0.0);
			limitImpulse = m_limitPositionImpulse - oldLimitImpulse;
		}

		b1->m_rotation -= b1->m_invI * limitImpulse;
		b1->m_R.Set(b1->m_rotation);
		b2->m_rotation += b2->m_invI * limitImpulse;
		b2->m_R.Set(b2->m_rotation);
	}

	return positionError <= b2_linearSlop && angularError <= b2_angularSlop;
}

b2Vec2 b2RevoluteJoint::GetAnchor1() const
{
	b2Body* b1 = m_body1;
	return b1->m_position + b2Mul(b1->m_R, m_localAnchor1);
}

b2Vec2 b2RevoluteJoint::GetAnchor2() const
{
	b2Body* b2 = m_body2;
	return b2->m_position + b2Mul(b2->m_R, m_localAnchor2);
}

float64 b2RevoluteJoint::GetJointAngle() const
{
	b2Body* b1 = m_body1;
	b2Body* b2 = m_body2;
	return b2->m_rotation - b1->m_rotation;
}

float64 b2RevoluteJoint::GetJointSpeed() const
{
	b2Body* b1 = m_body1;
	b2Body* b2 = m_body2;
	return b2->m_angularVelocity - b1->m_angularVelocity;
}

float64 b2RevoluteJoint::GetMotorTorque(float64 invTimeStep) const
{
	return invTimeStep * m_motorImpulse;
}

void b2RevoluteJoint::SetMotorSpeed(float64 speed)
{
	m_motorSpeed = speed;
}

void b2RevoluteJoint::SetMotorTorque(float64 torque)
{
	m_maxMotorTorque = torque;
}

b2Vec2 b2RevoluteJoint::GetReactionForce(float64 invTimeStep) const
{
	return invTimeStep * m_ptpImpulse;
}

float64 b2RevoluteJoint::GetReactionTorque(float64 invTimeStep) const
{
	return invTimeStep * m_limitImpulse;
}
