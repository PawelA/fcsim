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

#include <Dynamics/Contacts/b2Conservative.h>
#include <Collision/b2Collision.h>
#include <Collision/b2Shape.h>
#include <Dynamics/b2Body.h>


bool b2Conservative(b2Shape* shape1, b2Shape* shape2)
{
	b2Body* body1 = shape1->GetBody();
	b2Body* body2 = shape2->GetBody();

	b2Vec2 v1 = body1->m_position - body1->m_position0;
	float64 omega1 = body1->m_rotation - body1->m_rotation0;
	b2Vec2 v2 = body2->m_position - body2->m_position0;
	float64 omega2 = body2->m_rotation - body2->m_rotation0;

	float64 r1 = shape1->GetMaxRadius();
	float64 r2 = shape2->GetMaxRadius();

	b2Vec2 p1Start = body1->m_position0;
	float64 a1Start = body1->m_rotation0;

	b2Vec2 p2Start = body2->m_position0;
	float64 a2Start = body2->m_rotation0;

	b2Vec2 p1 = p1Start;
	float64 a1 = a1Start;
	b2Vec2 p2 = p2Start;
	float64 a2 = a2Start;

	b2Mat22 R1, R2;
	b2Mat22_SetAngle(&R1, a1);
	b2Mat22_SetAngle(&R2, a2);

	shape1->QuickSync(p1, R1);
	shape2->QuickSync(p2, R2);

	float64 s1 = 0.0;
	const int32 maxIterations = 10;
	b2Vec2 d;
	float64 invRelativeVelocity = 0.0;
	bool hit = true;
	b2Vec2 x1, x2;
	for (int32 iter = 0; iter < maxIterations; ++iter)
	{
		// Get the accurate distance between shapes.
		float64 distance = b2Distance(&x1, &x2, shape1, shape2);
		if (distance < b2_linearSlop)
		{
			if (iter == 0)
			{
				hit = false;
			}
			else
			{
				hit = true;
			}
			break;
		}

		if (iter == 0)
		{
			b2Vec2 d = x2 - x1;
			b2Vec2_Normalize(&d);
			float64 relativeVelocity = b2Dot(d, v1 - v2) + b2Abs(omega1) * r1 + b2Abs(omega2) * r2;
			if (b2Abs(relativeVelocity) < MIN_VALUE)
			{
				hit = false;
				break;
			}

			invRelativeVelocity = 1.0 / relativeVelocity;
		}

		// Get the conservative movement.
		float64 ds = distance * invRelativeVelocity;
		float64 s2 = s1 + ds;

		if (s2 < 0.0 || 1.0 < s2)
		{
			hit = false;
			break;
		}

		if (s2 < (1.0 + 100.0 * MIN_VALUE) * s1)
		{
			hit = true;
			break;
		}

		s1 = s2;

		// Move forward conservatively.
		p1 = p1Start + s1 * v1;
		a1 = a1Start + s1 * omega1;
		p2 = p2Start + s1 * v2;
		a2 = a2Start + s1 * omega2;

		b2Mat22_SetAngle(&R1, a1);
		b2Mat22_SetAngle(&R2, a2);
		shape1->QuickSync(p1, R1);
		shape2->QuickSync(p2, R2);
	}

	if (hit)
	{
		// Hit, move bodies to safe position and re-sync shapes.
		b2Vec2 d = x2 - x1;
		float64 length = b2Vec2_Length(&d);
		if (length > MIN_VALUE)
		{
			d *= b2_linearSlop / length;
		}

		if (b2Body_IsStatic(body1))
		{
			body1->m_position = p1;
		}
		else
		{
			body1->m_position = p1 - d;
		}
		body1->m_rotation = a1;
		b2Mat22_SetAngle(&body1->m_R, a1);
		body1->QuickSyncShapes();

		if (b2Body_IsStatic(body2))
		{
			body2->m_position = p2;
		}
		else
		{
			body2->m_position = p2 + d;
		}
		body2->m_position = p2 + d;
		body2->m_rotation = a2;
		b2Mat22_SetAngle(&body2->m_R, a2);
		body2->QuickSyncShapes();

		return true;
	}

	// No hit, restore shapes.
	shape1->QuickSync(body1->m_position, body1->m_R);
	shape2->QuickSync(body2->m_position, body2->m_R);
	return false;
}
