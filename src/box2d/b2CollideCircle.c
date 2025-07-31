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

#include <box2d/b2Vec.h>
#include <box2d/b2CMath.h>
#include <box2d/b2Collision.h>
#include <box2d/b2Shape.h>
#include <float.h>

void b2CollideCircle(b2Manifold* manifold, b2CircleShape* circle1, b2CircleShape* circle2)
{
	manifold->pointCount = 0;

	b2Vec2 d;
	d.x = circle2->m_shape.m_position.x - circle1->m_shape.m_position.x;
	d.y = circle2->m_shape.m_position.y - circle1->m_shape.m_position.y;
	float64 distSqr = b2Dot(d, d);
	float64 radiusSum = circle1->m_radius + circle2->m_radius;
	if (distSqr > radiusSum * radiusSum)
	{
		return;
	}

	float64 separation;
	if (distSqr < MIN_VALUE)
	{
		separation = -radiusSum;
		b2Vec2_Set(&manifold->normal, 0.0, 1.0);
	}
	else
	{
		float64 dist = sqrt(distSqr);
		separation = dist - radiusSum;
		float64 a = 1.0 / dist;
		manifold->normal.x = a * d.x;
		manifold->normal.y = a * d.y;
	}

	manifold->pointCount = 1;
	manifold->points[0].id.key = 0;
	manifold->points[0].separation = separation;
	manifold->points[0].position.x = circle2->m_shape.m_position.x - circle2->m_radius * manifold->normal.x;
	manifold->points[0].position.y = circle2->m_shape.m_position.y - circle2->m_radius * manifold->normal.y;
}

void b2CollidePolyAndCircle(b2Manifold* manifold, const b2PolyShape* poly, const b2CircleShape* circle)
{
	manifold->pointCount = 0;

	// Compute circle position in the frame of the polygon.
	b2Vec2 diff;
	diff.x = circle->m_shape.m_position.x - poly->m_shape.m_position.x;
	diff.y = circle->m_shape.m_position.y - poly->m_shape.m_position.y;
	b2Vec2 xLocal = b2MulT(poly->m_shape.m_R, diff);

	// Find the min separating edge.
	int32 normalIndex = 0;
	float64 separation = -DBL_MAX;
	const float64 radius = circle->m_radius;
	for (int32 i = 0; i < poly->m_vertexCount; ++i)
	{
		diff.x = xLocal.x - poly->m_vertices[i].x;
		diff.y = xLocal.y - poly->m_vertices[i].y;
		float64 s = b2Dot(poly->m_normals[i], diff);
		if (s > radius)
		{
			// Early out.
			return;
		}

		if (s > separation)
		{
			separation = s;
			normalIndex = i;
		}
	}

	// If the center is inside the polygon ...
	if (separation < MIN_VALUE)
	{
		manifold->pointCount = 1;
		manifold->normal = b2Mul(poly->m_shape.m_R, poly->m_normals[normalIndex]);
		manifold->points[0].id.features.incidentEdge = (uint8)normalIndex;
		manifold->points[0].id.features.incidentVertex = b2_nullFeature;
		manifold->points[0].id.features.referenceFace = b2_nullFeature;
		manifold->points[0].id.features.flip = 0;
		manifold->points[0].position.x = circle->m_shape.m_position.x - radius * manifold->normal.x;
		manifold->points[0].position.y = circle->m_shape.m_position.y - radius * manifold->normal.y;
		manifold->points[0].separation = separation - radius;
		return;
	}

	// Project the circle center onto the edge segment.
	int32 vertIndex1 = normalIndex;
	int32 vertIndex2 = vertIndex1 + 1 < poly->m_vertexCount ? vertIndex1 + 1 : 0;
	b2Vec2 e;
	e.x = poly->m_vertices[vertIndex2].x - poly->m_vertices[vertIndex1].x;
	e.y = poly->m_vertices[vertIndex2].y - poly->m_vertices[vertIndex1].y;
	float64 length = b2Vec2_Length(&e);
	e.x /= length;
	e.y /= length;

	// If the edge length is zero ...
	if (length < MIN_VALUE)
	{
		b2Vec2 d;
		d.x = xLocal.x - poly->m_vertices[vertIndex1].x;
		d.y = xLocal.y - poly->m_vertices[vertIndex1].y;
		float64 dist = b2Vec2_Length(&d);
		d.x /= dist;
		d.y /= dist;
		if (dist > radius)
		{
			return;
		}

		manifold->pointCount = 1;
		manifold->normal = b2Mul(poly->m_shape.m_R, d);
		manifold->points[0].id.features.incidentEdge = b2_nullFeature;
		manifold->points[0].id.features.incidentVertex = (uint8)vertIndex1;
		manifold->points[0].id.features.referenceFace = b2_nullFeature;
		manifold->points[0].id.features.flip = 0;
		manifold->points[0].position.x = circle->m_shape.m_position.x - radius * manifold->normal.x;
		manifold->points[0].position.y = circle->m_shape.m_position.y - radius * manifold->normal.y;
		manifold->points[0].separation = dist - radius;
		return;
	}

	// Project the center onto the edge.
	diff.x = xLocal.x - poly->m_vertices[vertIndex1].x;
	diff.y = xLocal.y - poly->m_vertices[vertIndex1].y;
	float64 u = b2Dot(diff, e);
	manifold->points[0].id.features.incidentEdge = b2_nullFeature;
	manifold->points[0].id.features.incidentVertex = b2_nullFeature;
	manifold->points[0].id.features.referenceFace = b2_nullFeature;
	manifold->points[0].id.features.flip = 0;
	b2Vec2 p;
	if (u <= 0.0)
	{
		p = poly->m_vertices[vertIndex1];
		manifold->points[0].id.features.incidentVertex = (uint8)vertIndex1;
	}
	else if (u >= length)
	{
		p = poly->m_vertices[vertIndex2];
		manifold->points[0].id.features.incidentVertex = (uint8)vertIndex2;
	}
	else
	{
		p.x = poly->m_vertices[vertIndex1].x + u * e.x;
		p.y = poly->m_vertices[vertIndex1].y + u * e.y;
		manifold->points[0].id.features.incidentEdge = (uint8)vertIndex1;
	}

	b2Vec2 d;
	d.x = xLocal.x - p.x;
	d.y = xLocal.y - p.y;
	float64 dist = b2Vec2_Length(&d);
	d.x /= dist;
	d.y /= dist;
	if (dist > radius)
	{
		return;
	}

	manifold->pointCount = 1;
	manifold->normal = b2Mul(poly->m_shape.m_R, d);
	manifold->points[0].position.x = circle->m_shape.m_position.x - radius * manifold->normal.x;
	manifold->points[0].position.y = circle->m_shape.m_position.y - radius * manifold->normal.y;
	manifold->points[0].separation = dist - radius;
}
