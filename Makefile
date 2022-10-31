OBJS = \
	draw.o \
	fcsim.o \
	main.o

B2_OBJS = \
	box2d/Source/Collision/b2BroadPhase.o \
	box2d/Source/Collision/b2CollideCircle.o \
	box2d/Source/Collision/b2CollidePoly.o \
	box2d/Source/Collision/b2Distance.o \
	box2d/Source/Collision/b2PairManager.o \
	box2d/Source/Collision/b2Shape.o \
	box2d/Source/Common/b2BlockAllocator.o \
	box2d/Source/Common/b2Settings.o \
	box2d/Source/Common/b2StackAllocator.o \
	box2d/Source/Dynamics/b2Body.o \
	box2d/Source/Dynamics/b2ContactManager.o \
	box2d/Source/Dynamics/b2Island.o \
	box2d/Source/Dynamics/b2WorldCallbacks.o \
	box2d/Source/Dynamics/b2World.o \
	box2d/Source/Dynamics/Contacts/b2CircleContact.o \
	box2d/Source/Dynamics/Contacts/b2Conservative.o \
	box2d/Source/Dynamics/Contacts/b2Contact.o \
	box2d/Source/Dynamics/Contacts/b2ContactSolver.o \
	box2d/Source/Dynamics/Contacts/b2PolyAndCircleContact.o \
	box2d/Source/Dynamics/Contacts/b2PolyContact.o \
	box2d/Source/Dynamics/Joints/b2DistanceJoint.o \
	box2d/Source/Dynamics/Joints/b2GearJoint.o \
	box2d/Source/Dynamics/Joints/b2Joint.o \
	box2d/Source/Dynamics/Joints/b2MouseJoint.o \
	box2d/Source/Dynamics/Joints/b2PrismaticJoint.o \
	box2d/Source/Dynamics/Joints/b2PulleyJoint.o \
	box2d/Source/Dynamics/Joints/b2RevoluteJoint.o

LDFLAGS = -lglfw -lGL

main: $(OBJS) box2d.a
	$(CXX) $(LDFLAGS) -o $@ $^

box2d.a: $(B2_OBJS)
	ar rc $@ $^
