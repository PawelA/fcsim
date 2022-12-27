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

#include "b2Collision.h"
#include "b2Shape.h"

struct ClipVertex
{
	b2Vec2 v;
	b2ContactID id;
};

static int32 ClipSegmentToLine(ClipVertex vOut[2], ClipVertex vIn[2],
					  const b2Vec2& normal, float64 offset)
{
	mw("clip_segment_to_line0", 7, vIn[0].v.x, vIn[0].v.y,
				       vIn[1].v.x, vIn[1].v.y,
				       normal.x, normal.y,
				       offset);

	// Start with no output points
	int32 numOut = 0;

	// Calculate the distance of end points to the line
	float64 distance0 = b2Dot(normal, vIn[0].v) - offset;
	float64 distance1 = b2Dot(normal, vIn[1].v) - offset;

	// If the points are behind the plane
	if (distance0 <= 0.0) vOut[numOut++] = vIn[0];
	if (distance1 <= 0.0) vOut[numOut++] = vIn[1];

	// If the points are on different sides of the plane
	if (distance0 * distance1 < 0.0)
	{
		// Find intersection point of edge and plane
		float64 interp = distance0 / (distance0 - distance1);
		vOut[numOut].v = vIn[0].v + interp * (vIn[1].v - vIn[0].v);
		if (distance0 > 0.0)
		{
			vOut[numOut].id = vIn[0].id;
		}
		else
		{
			vOut[numOut].id = vIn[1].id;
		}
		++numOut;
	}

	for (int i = 0; i < numOut; i++)
		mw("clip_segment_to_line1", 2, vOut[i].v.x, vOut[i].v.y);

	return numOut;
}

// Find the separation between poly1 and poly2 for a give edge normal on poly1.
static float64 EdgeSeparation(const b2PolyShape* poly1, int32 edge1, const b2PolyShape* poly2)
{
	const b2Vec2* vert1s = poly1->m_vertices;
	int32 count2 = poly2->m_vertexCount;
	const b2Vec2* vert2s = poly2->m_vertices;

	// Convert normal from into poly2's frame.
	b2Assert(edge1 < poly1->m_vertexCount);
	b2Vec2 normal = b2Mul(poly1->m_R, poly1->m_normals[edge1]);
	b2Vec2 normalLocal2 = b2MulT(poly2->m_R, normal);

	// Find support vertex on poly2 for -normal.
	int32 vertexIndex2 = 0;
	float64 minDot = DBL_MAX;
	for (int32 i = 0; i < count2; ++i)
	{
		float64 dot = b2Dot(vert2s[i], normalLocal2);
		if (dot < minDot)
		{
			minDot = dot;
			vertexIndex2 = i;
		}
	}

	b2Vec2 v1 = poly1->m_position + b2Mul(poly1->m_R, vert1s[edge1]);
	b2Vec2 v2 = poly2->m_position + b2Mul(poly2->m_R, vert2s[vertexIndex2]);
	float64 separation = b2Dot(v2 - v1, normal);
	return separation;
}

// Find the max separation between poly1 and poly2 using edge normals from poly1.
static float64 FindMaxSeparation(int32* edgeIndex, const b2PolyShape* poly1, const b2PolyShape* poly2, bool conservative)
{
	int32 count1 = poly1->m_vertexCount;

	// Vector pointing from the origin of poly1 to the origin of poly2.
	b2Vec2 d = poly2->m_position - poly1->m_position;
	b2Vec2 dLocal1 = b2MulT(poly1->m_R, d);

	// Find edge normal on poly1 that has the largest projection onto d.
	int32 edge = 0;
	float64 maxDot = -DBL_MAX;
	for (int32 i = 0; i < count1; ++i)
	{
		float64 dot = b2Dot(poly1->m_normals[i], dLocal1);
		if (dot > maxDot)
		{
			maxDot = dot;
			edge = i;
		}
	}

	// Get the separation for the edge normal.
	float64 s = EdgeSeparation(poly1, edge, poly2);
	if (s > 0.0 && conservative == false)
	{
		return s;
	}

	// Check the separation for the neighboring edges.
	int32 prevEdge = edge - 1 >= 0 ? edge - 1 : count1 - 1;
	float64 sPrev = EdgeSeparation(poly1, prevEdge, poly2);
	if (sPrev > 0.0 && conservative == false)
	{
		return sPrev;
	}

	int32 nextEdge = edge + 1 < count1 ? edge + 1 : 0;
	float64 sNext = EdgeSeparation(poly1, nextEdge, poly2);
	if (sNext > 0.0 && conservative == false)
	{
		return sNext;
	}

	// Find the best edge and the search direction.
	int32 bestEdge;
	float64 bestSeparation;
	int32 increment;
	if (sPrev > s && sPrev > sNext)
	{
		increment = -1;
		bestEdge = prevEdge;
		bestSeparation = sPrev;
	}
	else if (sNext > s)
	{
		increment = 1;
		bestEdge = nextEdge;
		bestSeparation = sNext;
	}
	else
	{
		*edgeIndex = edge;
		return s;
	}

	for ( ; ; )
	{
		if (increment == -1)
			edge = bestEdge - 1 >= 0 ? bestEdge - 1 : count1 - 1;
		else
			edge = bestEdge + 1 < count1 ? bestEdge + 1 : 0;

		s = EdgeSeparation(poly1, edge, poly2);
		if (s > 0.0 && conservative == false)
		{
			return s;
		}

		if (s > bestSeparation)
		{
			bestEdge = edge;
			bestSeparation = s;
		}
		else
		{
			break;
		}
	}

	*edgeIndex = bestEdge;
	return bestSeparation;
}

static void FindIncidentEdge(ClipVertex c[2], const b2PolyShape* poly1, int32 edge1, const b2PolyShape* poly2)
{
	int32 count1 = poly1->m_vertexCount;
	const b2Vec2* vert1s = poly1->m_vertices;
	int32 count2 = poly2->m_vertexCount;
	const b2Vec2* vert2s = poly2->m_vertices;

	// Get the vertices associated with edge1.
	int32 vertex11 = edge1;
	int32 vertex12 = edge1 + 1 == count1 ? 0 : edge1 + 1;

	// Get the normal of edge1.
	b2Vec2 normal1Local1 = b2Cross(vert1s[vertex12] - vert1s[vertex11], 1.0);
	float64 normal1Local1LenInv = 1.0 / normal1Local1.Length();
	normal1Local1.x *= normal1Local1LenInv;
	normal1Local1.y *= normal1Local1LenInv;
	b2Vec2 normal1 = b2Mul(poly1->m_R, normal1Local1);
	b2Vec2 normal1Local2 = b2MulT(poly2->m_R, normal1);

	// Find the incident edge on poly2.
	int32 vertex21 = 0, vertex22 = 0;
	float64 minDot = DBL_MAX;
	for (int32 i = 0; i < count2; ++i)
	{
		int32 i1 = i;
		int32 i2 = i + 1 < count2 ? i + 1 : 0;

		b2Vec2 normal2Local2 = b2Cross(vert2s[i2] - vert2s[i1], 1.0);
		float64 normal2Local2LenInv = 1.0 / normal2Local2.Length();
		normal2Local2.x *= normal2Local2LenInv;
		normal2Local2.y *= normal2Local2LenInv;
		float64 dot = b2Dot(normal2Local2, normal1Local2);
		if (dot < minDot)
		{
			minDot = dot;
			vertex21 = i1;
			vertex22 = i2;
		}
	}

	// Build the clip vertices for the incident edge.
	c[0].v = poly2->m_position + b2Mul(poly2->m_R, vert2s[vertex21]);
	c[0].id.features.referenceFace = (uint8)edge1;
	c[0].id.features.incidentEdge = (uint8)vertex21;
	c[0].id.features.incidentVertex = (uint8)vertex21;

	c[1].v = poly2->m_position + b2Mul(poly2->m_R, vert2s[vertex22]);
	c[1].id.features.referenceFace = (uint8)edge1;
	c[1].id.features.incidentEdge = (uint8)vertex21;
	c[1].id.features.incidentVertex = (uint8)vertex22;
}

// Find edge normal of max separation on A - return if separating axis is found
// Find edge normal of max separation on B - return if separation axis is found
// Choose reference edge as min(minA, minB)
// Find incident edge
// Clip

// The normal points from 1 to 2
void b2CollidePoly(b2Manifold* manifold, const b2PolyShape* polyA, const b2PolyShape* polyB, bool conservative)
{
	mw("collide_poly0", 0);
	NOT_USED(conservative);

	manifold->pointCount = 0;

	int32 edgeA = 0;
	float64 separationA = FindMaxSeparation(&edgeA, polyA, polyB, conservative);
	if (separationA > 0.0 && conservative == false)
		return;
	
	mw("collide_poly1", 0);

	int32 edgeB = 0;
	float64 separationB = FindMaxSeparation(&edgeB, polyB, polyA, conservative);
	if (separationB > 0.0 && conservative == false)
		return;

	mw("collide_poly2", 0);

	const b2PolyShape* poly1;	// reference poly
	const b2PolyShape* poly2;	// incident poly
	int32 edge1;		// reference edge
	uint8 flip;
	const float64 k_relativeTol = 0.98;
	const float64 k_absoluteTol = 0.001;

	// TODO_ERIN use "radius" of poly for absolute tolerance.
	if (separationB > k_relativeTol * separationA + k_absoluteTol)
	{
		poly1 = polyB;
		poly2 = polyA;
		edge1 = edgeB;
		flip = 1;
	}
	else
	{
		poly1 = polyA;
		poly2 = polyB;
		edge1 = edgeA;
		flip = 0;
	}

	ClipVertex incidentEdge[2];
	FindIncidentEdge(incidentEdge, poly1, edge1, poly2);

	int32 count1 = poly1->m_vertexCount;
	const b2Vec2* vert1s = poly1->m_vertices;

	b2Vec2 v11 = vert1s[edge1];
	b2Vec2 v12 = edge1 + 1 < count1 ? vert1s[edge1+1] : vert1s[0];

	b2Vec2 dv = v12 - v11;
	b2Vec2 sideNormal = b2Mul(poly1->m_R, v12 - v11);
	mw("side_normal", 2, sideNormal.x, sideNormal.y);
	float64 sideNormalLenInv = 1.0 / sideNormal.Length();
	sideNormal.x *= sideNormalLenInv;
	sideNormal.y *= sideNormalLenInv;
	b2Vec2 frontNormal = b2Cross(sideNormal, 1.0);

	v11 = poly1->m_position + b2Mul(poly1->m_R, v11);
	v12 = poly1->m_position + b2Mul(poly1->m_R, v12);

	float64 frontOffset = b2Dot(frontNormal, v11);
	float64 sideOffset1 = -b2Dot(sideNormal, v11);
	float64 sideOffset2 = b2Dot(sideNormal, v12);

	// Clip incident edge against extruded edge1 side edges.
	ClipVertex clipPoints1[2];
	ClipVertex clipPoints2[2];
	int np;

	// Clip to box side 1
	np = ClipSegmentToLine(clipPoints1, incidentEdge, -sideNormal, sideOffset1);

	if (np < 2)
		return;
	
	mw("collide_poly3", 0);

	// Clip to negative box side 1
	np = ClipSegmentToLine(clipPoints2, clipPoints1,  sideNormal, sideOffset2);

	if (np < 2)
		return;
	
	mw("collide_poly4", 0);

	// Now clipPoints2 contains the clipped points.
	manifold->normal = flip ? -frontNormal : frontNormal;

	int32 pointCount = 0;
	for (int32 i = 0; i < b2_maxManifoldPoints; ++i)
	{
		float64 separation = b2Dot(frontNormal, clipPoints2[i].v) - frontOffset;

		if (separation <= 0.0 || conservative == true)
		{
			b2ContactPoint* cp = manifold->points + pointCount;
			cp->separation = separation;
			cp->position = clipPoints2[i].v;
			cp->id = clipPoints2[i].id;
			cp->id.features.flip = flip;
			++pointCount;
		}
	}

	manifold->pointCount = pointCount;
}
