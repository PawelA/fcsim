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

#ifndef B2_SHAPE_H
#define B2_SHAPE_H

#include "../Common/b2Math.h"
#include "b2Collision.h"

struct b2Body;
class b2BroadPhase;

struct b2MassData
{
	float64 mass;
	b2Vec2 center;
	float64 I;
};

enum b2ShapeType
{
	e_unknownShape = -1,
	e_circleShape,
	e_boxShape,
	e_polyShape,
	e_meshShape,
	e_shapeTypeCount,
};

struct b2ShapeDef
{
	void ComputeMass(b2MassData* massData) const;

	b2ShapeType type;
	void* userData;
	b2Vec2 localPosition;
	float64 localRotation;
	float64 friction;
	float64 restitution;
	float64 density;

	// The collision category bits. Normally you would just set one bit.
	uint16 categoryBits;

	// The collision mask bits. This states the categories that this
	// shape would accept for collision.
	uint16 maskBits;

	// Collision groups allow a certain group of objects to never collide (negative)
	// or always collide (positive). Zero means no collision group. Non-zero group
	// filtering always wins against the mask bits.
	int16 groupIndex;
};

static void b2ShapeDef_ctor(b2ShapeDef *def)
{
	def->type = e_unknownShape;
	def->userData = NULL;
	b2Vec2_Set(&def->localPosition, 0.0, 0.0);
	def->localRotation = 0.0;
	def->friction = 0.2;
	def->restitution = 0.0;
	def->density = 0.0;
	def->categoryBits = 0x0001;
	def->maskBits = 0xFFFF;
	def->groupIndex = 0;
}

struct b2CircleDef
{
	b2CircleDef()
	{
		b2ShapeDef_ctor(&m_shapeDef);
		m_shapeDef.type = e_circleShape;
		radius = 1.0;
	}

	b2ShapeDef m_shapeDef;

	float64 radius;
};

struct b2BoxDef
{
	b2BoxDef()
	{
		b2ShapeDef_ctor(&m_shapeDef);
		m_shapeDef.type = e_boxShape;
		b2Vec2_Set(&extents, 1.0, 1.0);
	}

	b2ShapeDef m_shapeDef;

	b2Vec2 extents;
};

// Convex polygon, vertices must be in CCW order.
struct b2PolyDef
{
	b2PolyDef()
	{
		b2ShapeDef_ctor(&m_shapeDef);
		m_shapeDef.type = e_polyShape;
		vertexCount = 0;
	}

	b2ShapeDef m_shapeDef;

	b2Vec2 vertices[b2_maxPolyVertices];
	int32 vertexCount;
};

// Shapes are created automatically when a body is created.
// Client code does not normally interact with shapes.
class b2Shape
{
public:
	virtual bool TestPoint(const b2Vec2& p) = 0;

	void* GetUserData();

	b2ShapeType GetType() const;

	// Get the parent body of this shape.
	b2Body* GetBody();

	// Get the world position.
	const b2Vec2& GetPosition() const;

	// Get the world rotation.
	const b2Mat22& GetRotationMatrix() const;

	// Remove and then add proxy from the broad-phase.
	// This is used to refresh the collision filters.
	virtual void ResetProxy(b2BroadPhase* broadPhase) = 0;

	// Get the next shape in the parent body's shape list.
	b2Shape* GetNext();

	//--------------- Internals Below -------------------

	static b2Shape* Create(	const b2ShapeDef* def,
							b2Body* body, const b2Vec2& newOrigin);

	static void Destroy(b2Shape*& shape);

	b2Shape(const b2ShapeDef* def, b2Body* body);

	virtual ~b2Shape();

	virtual void Synchronize(	const b2Vec2& position1, const b2Mat22& R1,
								const b2Vec2& position2, const b2Mat22& R2) = 0;
	virtual void QuickSync(const b2Vec2& position, const b2Mat22& R) = 0;

	virtual b2Vec2 Support(const b2Vec2& d) const = 0;
	float64 GetMaxRadius() const;

	void DestroyProxy();

	b2Shape* m_next;

	b2Mat22 m_R;
	b2Vec2 m_position;

	b2ShapeType m_type;

	void* m_userData;

	b2Body* m_body;

	float64 m_friction;
	float64 m_restitution;

	float64 m_maxRadius;

	uint16 m_proxyId;
	uint16 m_categoryBits;
	uint16 m_maskBits;
	int16 m_groupIndex;
};

class b2CircleShape : public b2Shape
{
public:
	bool TestPoint(const b2Vec2& p);

	void ResetProxy(b2BroadPhase* broadPhase);

	//--------------- Internals Below -------------------

	b2CircleShape(const b2ShapeDef* def, b2Body* body, const b2Vec2& newOrigin);

	void Synchronize(	const b2Vec2& position1, const b2Mat22& R1,
						const b2Vec2& position2, const b2Mat22& R2);
	void QuickSync(const b2Vec2& position, const b2Mat22& R);

	b2Vec2 Support(const b2Vec2& d) const;

	// Local position in parent body
	b2Vec2 m_localPosition;
	float64 m_radius;
};

// A convex polygon. The position of the polygon (m_position) is the
// position of the centroid. The vertices of the incoming polygon are pre-rotated
// according to the local rotation. The vertices are also shifted to be centered
// on the centroid. Since the local rotation is absorbed into the vertex
// coordinates, the polygon rotation is equal to the body rotation. However,
// the polygon position is centered on the polygon centroid. This simplifies
// some collision algorithms.
class b2PolyShape : public b2Shape
{
public:
	bool TestPoint(const b2Vec2& p);

	void ResetProxy(b2BroadPhase* broadPhase);

	//--------------- Internals Below -------------------

	b2PolyShape(const b2ShapeDef* def, b2Body* body, const b2Vec2& newOrigin);

	void Synchronize(	const b2Vec2& position1, const b2Mat22& R1,
						const b2Vec2& position2, const b2Mat22& R2);
	void QuickSync(const b2Vec2& position, const b2Mat22& R);

	b2Vec2 Support(const b2Vec2& d) const;

	// Local position of the shape centroid in parent body frame.
	b2Vec2 m_localCentroid;

	// Local position oriented bounding box. The OBB center is relative to
	// shape centroid.
	b2OBB m_localOBB;
	b2Vec2 m_vertices[b2_maxPolyVertices];
	b2Vec2 m_coreVertices[b2_maxPolyVertices];
	int32 m_vertexCount;
	b2Vec2 m_normals[b2_maxPolyVertices];
};

inline b2ShapeType b2Shape::GetType() const
{
	return m_type;
}

inline void* b2Shape::GetUserData()
{
	return m_userData;
}

inline b2Body* b2Shape::GetBody()
{
	return m_body;
}

inline b2Shape* b2Shape::GetNext()
{
	return m_next;
}

inline const b2Vec2& b2Shape::GetPosition() const
{
	return m_position;
}

inline const b2Mat22& b2Shape::GetRotationMatrix() const
{
	return m_R;
}

inline float64 b2Shape::GetMaxRadius() const
{
	return m_maxRadius;
}


#endif
