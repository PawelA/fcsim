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

#include <Dynamics/Joints/b2Joint.h>
#include <Dynamics/Joints/b2RevoluteJoint.h>
#include <Dynamics/b2Body.h>
#include <Dynamics/b2World.h>
#include <Common/b2BlockAllocator.h>
#include <Collision/b2BroadPhase.h>

#include <new>

b2Joint* b2Joint::Create(const b2JointDef* def, b2BlockAllocator* allocator)
{
	b2Joint* joint = NULL;

	switch (def->type)
	{
	case e_revoluteJoint:
		{
			b2RevoluteJoint* revoluteJoint = (b2RevoluteJoint *)b2BlockAllocator_Allocate(allocator, sizeof(b2RevoluteJoint));
			b2RevoluteJoint_ctor(revoluteJoint, (b2RevoluteJointDef*)def);
			joint = &revoluteJoint->m_joint;
		}
		break;

	default:
		b2Assert(false);
		break;
	}

	return joint;
}

void b2Joint::Destroy(b2Joint* joint, b2BlockAllocator* allocator)
{
	joint->~b2Joint();
	switch (joint->m_type)
	{
	case e_revoluteJoint:
		b2BlockAllocator_Free(allocator, joint, sizeof(b2RevoluteJoint));
		break;

	default:
		b2Assert(false);
		break;
	}
}

b2Joint::b2Joint(const b2JointDef* def)
{
	m_type = def->type;
	m_prev = NULL;
	m_next = NULL;
	m_body1 = def->body1;
	m_body2 = def->body2;
	m_collideConnected = def->collideConnected;
	m_islandFlag = false;
	m_userData = def->userData;
}
