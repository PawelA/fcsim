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

#include <box2d/b2CircleContact.h>
#include <box2d/b2BlockAllocator.h>

#include <new>

b2Contact* b2CircleContact::Create(b2Shape* shape1, b2Shape* shape2, b2BlockAllocator* allocator)
{
	b2CircleContact *circ_contact = (b2CircleContact *)b2BlockAllocator_Allocate(allocator, sizeof(b2CircleContact));
	new (circ_contact) b2CircleContact(shape1, shape2);
	return &circ_contact->contact;
}

void b2CircleContact::Destroy(b2Contact* contact, b2BlockAllocator* allocator)
{
	((b2CircleContact*)contact)->~b2CircleContact();
	b2BlockAllocator_Free(allocator, contact, sizeof(b2CircleContact));
}

static void b2CircleContact_Evaluate(b2Contact *contact);

static b2Manifold *b2CircleContact_GetManifolds(b2Contact *contact)
{
	b2CircleContact *circ_contact = (b2CircleContact *)contact;
	return &circ_contact->m_manifold;
}

b2CircleContact::b2CircleContact(b2Shape* s1, b2Shape* s2)
{
	b2Contact_ctor(&contact, s1, s2);
	contact.Evaluate = b2CircleContact_Evaluate;
	contact.GetManifolds = b2CircleContact_GetManifolds;
	b2Assert(m_shape1->m_type == e_circleShape);
	b2Assert(m_shape2->m_type == e_circleShape);
	m_manifold.pointCount = 0;
	m_manifold.points[0].normalImpulse = 0.0;
	m_manifold.points[0].tangentImpulse = 0.0;
}

static void b2CircleContact_Evaluate(b2Contact *contact)
{
	b2CircleContact *circ_contact = (b2CircleContact *)contact;
	b2CollideCircle(&circ_contact->m_manifold, (b2CircleShape*)contact->m_shape1, (b2CircleShape*)contact->m_shape2, false);

	if (circ_contact->m_manifold.pointCount > 0)
	{
		contact->m_manifoldCount = 1;
	}
	else
	{
		contact->m_manifoldCount = 0;
	}

}
