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

#include <Dynamics/Contacts/b2PolyContact.h>
#include <Common/b2BlockAllocator.h>

#include <cstring>
#include <memory>
#include <new>

b2Contact* b2PolyContact::Create(b2Shape* shape1, b2Shape* shape2, b2BlockAllocator* allocator)
{
	b2PolyContact *poly_contact = (b2PolyContact *)b2BlockAllocator_Allocate(allocator, sizeof(b2PolyContact));
	new (poly_contact) b2PolyContact(shape1, shape2);
	return &poly_contact->contact;
}

void b2PolyContact::Destroy(b2Contact* contact, b2BlockAllocator* allocator)
{
	((b2PolyContact*)contact)->~b2PolyContact();
	b2BlockAllocator_Free(allocator, contact, sizeof(b2PolyContact));
}

static void b2PolyContact_Evaluate(b2Contact *contact);
static b2Manifold *b2PolyContact_GetManifolds(b2Contact *contact)
{
	b2PolyContact *poly_contact = (b2PolyContact *)contact;
	return &poly_contact->m_manifold;
}

b2PolyContact::b2PolyContact(b2Shape* s1, b2Shape* s2)
	: contact(s1, s2)
{
	contact.Evaluate = b2PolyContact_Evaluate;
	contact.GetManifolds = b2PolyContact_GetManifolds;
	b2Assert(m_shape1->m_type == e_polyShape);
	b2Assert(m_shape2->m_type == e_polyShape);
	m_manifold.pointCount = 0;
}

void b2PolyContact_Evaluate(b2Contact *contact)
{
	b2PolyContact *poly_contact = (b2PolyContact *)contact;
	b2Manifold m0;
	memcpy(&m0, &poly_contact->m_manifold, sizeof(b2Manifold));

	b2CollidePoly(&poly_contact->m_manifold, (b2PolyShape*)contact->m_shape1, (b2PolyShape*)contact->m_shape2, false);

	// Match contact ids to facilitate warm starting.
	if (poly_contact->m_manifold.pointCount > 0)
	{
		bool match[b2_maxManifoldPoints] = {false, false};

		// Match old contact ids to new contact ids and copy the
		// stored impulses to warm start the solver.
		for (int32 i = 0; i < poly_contact->m_manifold.pointCount; ++i)
		{
			b2ContactPoint* cp = poly_contact->m_manifold.points + i;
			cp->normalImpulse = 0.0;
			cp->tangentImpulse = 0.0;
			b2ContactID id = cp->id;

			for (int32 j = 0; j < m0.pointCount; ++j)
			{
				if (match[j] == true)
					continue;

				b2ContactPoint* cp0 = m0.points + j;
				b2ContactID id0 = cp0->id;

				if (id0.key == id.key)
				{
					match[j] = true;
					poly_contact->m_manifold.points[i].normalImpulse = m0.points[j].normalImpulse;
					poly_contact->m_manifold.points[i].tangentImpulse = m0.points[j].tangentImpulse;
					break;
				}
			}
		}

		contact->m_manifoldCount = 1;
	}
	else
	{
		contact->m_manifoldCount = 0;
	}
}
