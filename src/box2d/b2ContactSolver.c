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

#include <stdlib.h>

#include <box2d/b2ContactSolver.h>
#include <box2d/b2Contact.h>
#include <box2d/b2Body.h>
#include <box2d/b2World.h>
#include <box2d/b2CMath.h>

void b2ContactSolver_ctor(b2ContactSolver *solver, b2Contact** contacts, int32 contactCount)
{
	solver->m_constraintCount = 0;
	for (int32 i = 0; i < contactCount; ++i)
	{
		solver->m_constraintCount += contacts[i]->m_manifoldCount;
	}

	solver->m_constraints = (b2ContactConstraint*)malloc(solver->m_constraintCount * sizeof(b2ContactConstraint));

	int32 count = 0;
	for (int32 i = 0; i < contactCount; ++i)
	{
		b2Contact* contact = contacts[i];
		b2Body* b1 = contact->m_shape1->m_body;
		b2Body* b2 = contact->m_shape2->m_body;
		int32 manifoldCount = contact->m_manifoldCount;
		b2Manifold* manifolds = contact->GetManifolds(contact);
		float64 friction = contact->m_friction;
		float64 restitution = contact->m_restitution;

		b2Vec2 v1 = b1->m_linearVelocity;
		b2Vec2 v2 = b2->m_linearVelocity;
		float64 w1 = b1->m_angularVelocity;
		float64 w2 = b2->m_angularVelocity;

		for (int32 j = 0; j < manifoldCount; ++j)
		{
			b2Manifold* manifold = manifolds + j;

			const b2Vec2 normal = manifold->normal;

			b2ContactConstraint* c = solver->m_constraints + count;
			c->body1 = b1;
			c->body2 = b2;
			c->manifold = manifold;
			c->normal = normal;
			c->pointCount = manifold->pointCount;
			c->friction = friction;
			c->restitution = restitution;

			for (int32 k = 0; k < c->pointCount; ++k)
			{
				b2ContactPoint* cp = manifold->points + k;
				b2ContactConstraintPoint* ccp = c->points + k;

				ccp->normalImpulse = cp->normalImpulse;
				ccp->tangentImpulse = cp->tangentImpulse;
				ccp->separation = cp->separation;
				unsigned long long dupa = 0x7fffffffe0000000LLU;
				ccp->positionImpulse = *(double *)&dupa;

				b2Vec2 r1;
				r1.x = cp->position.x - b1->m_position.x;
				r1.y = cp->position.y - b1->m_position.y;
				b2Vec2 r2;
				r2.x = cp->position.x - b2->m_position.x;
				r2.y = cp->position.y - b2->m_position.y;

				ccp->localAnchor1 = b2MulT(b1->m_R, r1);
				ccp->localAnchor2 = b2MulT(b2->m_R, r2);

				float64 r1Sqr = b2Dot(r1, r1);
				float64 r2Sqr = b2Dot(r2, r2);

				float64 rn1 = b2Dot(r1, normal);
				float64 rn2 = b2Dot(r2, normal);
				float64 kNormal = b1->m_invMass + b2->m_invMass;
				kNormal += b1->m_invI * (r1Sqr - rn1 * rn1) + b2->m_invI * (r2Sqr - rn2 * rn2);
				ccp->normalMass = 1.0 / kNormal;

				b2Vec2 tangent = b2Cross_vs(normal, 1.0);

				float64 rt1 = b2Dot(r1, tangent);
				float64 rt2 = b2Dot(r2, tangent);
				float64 kTangent = b1->m_invMass + b2->m_invMass;
				kTangent += b1->m_invI * (r1Sqr - rt1 * rt1) + b2->m_invI * (r2Sqr - rt2 * rt2);
				ccp->tangentMass = 1.0 /  kTangent;

				// Setup a velocity bias for restitution.
				ccp->velocityBias = 0.0;
				if (ccp->separation > 0.0)
				{
					ccp->velocityBias = -60.0 * ccp->separation; // TODO_ERIN b2TimeStep
				}

				b2Vec2 c2 = b2Cross(w2, r2);
				b2Vec2 c1 = b2Cross(w1, r1);
				b2Vec2 v;
				v.x = v2.x + c2.x - v1.x - c1.x;
				v.y = v2.y + c2.y - v1.y - c1.y;
				float64 vRel = b2Dot(c->normal, v);
				if (vRel < -b2_velocityThreshold)
				{
					ccp->velocityBias += -c->restitution * vRel;
				}
			}

			++count;
		}
	}
}

void b2ContactSolver_dtor(b2ContactSolver *solver)
{
	free(solver->m_constraints);
}

void b2ContactSolver_PreSolve(b2ContactSolver *solver)
{
	// Warm start.
	for (int32 i = 0; i < solver->m_constraintCount; ++i)
	{
		b2ContactConstraint* c = solver->m_constraints + i;

		b2Body* b1 = c->body1;
		b2Body* b2 = c->body2;
		float64 invMass1 = b1->m_invMass;
		float64 invI1 = b1->m_invI;
		float64 invMass2 = b2->m_invMass;
		float64 invI2 = b2->m_invI;
		b2Vec2 normal = c->normal;
		b2Vec2 tangent = b2Cross_vs(normal, 1.0);

		for (int32 j = 0; j < c->pointCount; ++j)
		{
			b2ContactConstraintPoint* ccp = c->points + j;
			b2Vec2 P;
			P.x = ccp->normalImpulse * normal.x + ccp->tangentImpulse * tangent.x;
			P.y = ccp->normalImpulse * normal.y + ccp->tangentImpulse * tangent.y;
			b2Vec2 r1 = b2Mul(b1->m_R, ccp->localAnchor1);
			b2Vec2 r2 = b2Mul(b2->m_R, ccp->localAnchor2);
			b1->m_angularVelocity -= invI1 * b2Cross_vv(r1, P);
			b1->m_linearVelocity.x -= invMass1 * P.x;
			b1->m_linearVelocity.y -= invMass1 * P.y;
			b2->m_angularVelocity += invI2 * b2Cross_vv(r2, P);
			b2->m_linearVelocity.x += invMass2 * P.x;
			b2->m_linearVelocity.y += invMass2 * P.y;

			ccp->positionImpulse = 0.0;
		}
	}
}

void b2ContactSolver_SolveVelocityConstraints(b2ContactSolver *solver)
{
	for (int32 i = 0; i < solver->m_constraintCount; ++i)
	{
		b2ContactConstraint* c = solver->m_constraints + i;
		b2Body* b1 = c->body1;
		b2Body* b2 = c->body2;
		float64 invMass1 = b1->m_invMass;
		float64 invI1 = b1->m_invI;
		float64 invMass2 = b2->m_invMass;
		float64 invI2 = b2->m_invI;
		b2Vec2 normal = c->normal;
		b2Vec2 tangent = b2Cross_vs(normal, 1.0);

		// Solver normal constraints
		for (int32 j = 0; j < c->pointCount; ++j)
		{
		{
			b2ContactConstraintPoint* ccp = c->points + j;

			b2Vec2 r1 = b2Mul(b1->m_R, ccp->localAnchor1);
			b2Vec2 r2 = b2Mul(b2->m_R, ccp->localAnchor2);

			// Relative velocity at contact
			b2Vec2 c1 = b2Cross(b1->m_angularVelocity, r1);
			b2Vec2 c2 = b2Cross(b2->m_angularVelocity, r2);
			b2Vec2 dv;
			dv.x = b2->m_linearVelocity.x + c2.x - b1->m_linearVelocity.x - c1.x;
			dv.y = b2->m_linearVelocity.y + c2.y - b1->m_linearVelocity.y - c1.y;

			// Compute normal impulse
			float64 vn = b2Dot(dv, normal);
			float64 lambda = -ccp->normalMass * (vn - ccp->velocityBias);

			// b2Clamp the accumulated impulse
			float64 newImpulse = b2Max(ccp->normalImpulse + lambda, 0.0);
			lambda = newImpulse - ccp->normalImpulse;

			// Apply contact impulse
			b2Vec2 P;
			P.x = lambda * normal.x;
			P.y = lambda * normal.y;

			b1->m_linearVelocity.x -= invMass1 * P.x;
			b1->m_linearVelocity.y -= invMass1 * P.y;
			b1->m_angularVelocity -= invI1 * b2Cross_vv(r1, P);

			b2->m_linearVelocity.x += invMass2 * P.x;
			b2->m_linearVelocity.y += invMass2 * P.y;
			b2->m_angularVelocity += invI2 * b2Cross_vv(r2, P);

			ccp->normalImpulse = newImpulse;
		}

		// Solver tangent constraints
		{
			b2ContactConstraintPoint* ccp = c->points + j;

			b2Vec2 r1 = b2Mul(b1->m_R, ccp->localAnchor1);
			b2Vec2 r2 = b2Mul(b2->m_R, ccp->localAnchor2);

			// Relative velocity at contact
			b2Vec2 c1 = b2Cross(b1->m_angularVelocity, r1);
			b2Vec2 c2 = b2Cross(b2->m_angularVelocity, r2);
			b2Vec2 dv;
			dv.x = b2->m_linearVelocity.x + c2.x - b1->m_linearVelocity.x - c1.x;
			dv.y = b2->m_linearVelocity.y + c2.y - b1->m_linearVelocity.y - c1.y;

			// Compute tangent impulse
			float64 vt = b2Dot(dv, tangent);
			float64 lambda = ccp->tangentMass * (-vt);

			// b2Clamp the accumulated impulse
			float64 maxFriction = c->friction * ccp->normalImpulse;
			float64 newImpulse = b2Clamp(ccp->tangentImpulse + lambda, -maxFriction, maxFriction);
			lambda = newImpulse - ccp->tangentImpulse;

			// Apply contact impulse
			b2Vec2 P;
			P.x = lambda * tangent.x;
			P.y = lambda * tangent.y;

			b1->m_linearVelocity.x -= invMass1 * P.x;
			b1->m_linearVelocity.y -= invMass1 * P.y;
			b1->m_angularVelocity -= invI1 * b2Cross_vv(r1, P);

			b2->m_linearVelocity.x += invMass2 * P.x;
			b2->m_linearVelocity.y += invMass2 * P.y;
			b2->m_angularVelocity += invI2 * b2Cross_vv(r2, P);

			ccp->tangentImpulse = newImpulse;
		}
		}
	}
}

bool b2ContactSolver_SolvePositionConstraints(b2ContactSolver *solver, float64 beta)
{
	float64 minSeparation = 0.0;

	for (int32 i = 0; i < solver->m_constraintCount; ++i)
	{
		b2ContactConstraint* c = solver->m_constraints + i;
		b2Body* b1 = c->body1;
		b2Body* b2 = c->body2;
		float64 invMass1 = b1->m_invMass;
		float64 invI1 = b1->m_invI;
		float64 invMass2 = b2->m_invMass;
		float64 invI2 = b2->m_invI;
		b2Vec2 normal = c->normal;
		b2Vec2 tangent = b2Cross_vs(normal, 1.0);

		// Solver normal constraints
		for (int32 j = 0; j < c->pointCount; ++j)
		{
			b2ContactConstraintPoint* ccp = c->points + j;

			b2Vec2 r1 = b2Mul(b1->m_R, ccp->localAnchor1);
			b2Vec2 r2 = b2Mul(b2->m_R, ccp->localAnchor2);

			b2Vec2 p1;
			p1.x = b1->m_position.x + r1.x;
			p1.y = b1->m_position.y + r1.y;
			b2Vec2 p2;
			p2.x = b2->m_position.x + r2.x;
			p2.y = b2->m_position.y + r2.y;
			b2Vec2 dp;
			dp.x = p2.x - p1.x;
			dp.y = p2.y - p1.y;

			// Approximate the current separation.
			float64 separation = b2Dot(dp, normal) + ccp->separation;

			// Track max constraint error.
			minSeparation = b2Min(minSeparation, separation);

			// Prevent large corrections and allow slop.
			float64 C = beta * b2Clamp(separation + b2_linearSlop, -b2_maxLinearCorrection, 0.0);

			// Compute normal impulse
			float64 dImpulse = -ccp->normalMass * C;

			// b2Clamp the accumulated impulse
			float64 impulse0 = ccp->positionImpulse;
			ccp->positionImpulse = b2Max(impulse0 + dImpulse, 0.0);
			dImpulse = ccp->positionImpulse - impulse0;

			b2Vec2 impulse;
			impulse.x = dImpulse * normal.x;
			impulse.y = dImpulse * normal.y;

			b1->m_position.x -= invMass1 * impulse.x;
			b1->m_position.y -= invMass1 * impulse.y;
			b1->m_rotation -= invI1 * b2Cross_vv(r1, impulse);
			b2Mat22_SetAngle(&b1->m_R, b1->m_rotation);

			b2->m_position.x += invMass2 * impulse.x;
			b2->m_position.y += invMass2 * impulse.y;
			b2->m_rotation += invI2 * b2Cross_vv(r2, impulse);
			b2Mat22_SetAngle(&b2->m_R, b2->m_rotation);
		}
	}

	return minSeparation >= -b2_linearSlop;
}

void b2ContactSolver_PostSolve(b2ContactSolver *solver)
{
	for (int32 i = 0; i < solver->m_constraintCount; ++i)
	{
		b2ContactConstraint* c = solver->m_constraints + i;
		b2Manifold* m = c->manifold;

		for (int32 j = 0; j < c->pointCount; ++j)
		{
			m->points[j].normalImpulse = c->points[j].normalImpulse;
			m->points[j].tangentImpulse = c->points[j].tangentImpulse;
		}
	}
}