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
#include <Dynamics/Joints/b2DistanceJoint.h>
#include <Dynamics/Joints/b2MouseJoint.h>
#include <Dynamics/Joints/b2RevoluteJoint.h>
#include <Dynamics/Joints/b2PrismaticJoint.h>
#include <Dynamics/Joints/b2PulleyJoint.h>
#include <Dynamics/Joints/b2GearJoint.h>
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
	case e_distanceJoint:
		{
			void* mem = b2BlockAllocator_Allocate(allocator, sizeof(b2DistanceJoint));
			joint = new (mem) b2DistanceJoint((b2DistanceJointDef*)def);
		}
		break;

	case e_mouseJoint:
		{
			void* mem = b2BlockAllocator_Allocate(allocator, sizeof(b2MouseJoint));
			joint = new (mem) b2MouseJoint((b2MouseJointDef*)def);
		}
		break;

	case e_prismaticJoint:
		{
			void* mem = b2BlockAllocator_Allocate(allocator, sizeof(b2PrismaticJoint));
			joint = new (mem) b2PrismaticJoint((b2PrismaticJointDef*)def);
		}
		break;

	case e_revoluteJoint:
		{
			void* mem = b2BlockAllocator_Allocate(allocator, sizeof(b2RevoluteJoint));
			joint = new (mem) b2RevoluteJoint((b2RevoluteJointDef*)def);
		}
		break;

	case e_pulleyJoint:
		{
			void* mem = b2BlockAllocator_Allocate(allocator, sizeof(b2PulleyJoint));
			joint = new (mem) b2PulleyJoint((b2PulleyJointDef*)def);
		}
		break;

	case e_gearJoint:
		{
			void* mem = b2BlockAllocator_Allocate(allocator, sizeof(b2GearJoint));
			joint = new (mem) b2GearJoint((b2GearJointDef*)def);
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
	case e_distanceJoint:
		b2BlockAllocator_Free(allocator, joint, sizeof(b2DistanceJoint));
		break;

	case e_mouseJoint:
		b2BlockAllocator_Free(allocator, joint, sizeof(b2MouseJoint));
		break;

	case e_prismaticJoint:
		b2BlockAllocator_Free(allocator, joint, sizeof(b2PrismaticJoint));
		break;

	case e_revoluteJoint:
		b2BlockAllocator_Free(allocator, joint, sizeof(b2RevoluteJoint));
		break;

	case e_pulleyJoint:
		b2BlockAllocator_Free(allocator, joint, sizeof(b2PulleyJoint));
		break;

	case e_gearJoint:
		b2BlockAllocator_Free(allocator, joint, sizeof(b2GearJoint));
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
