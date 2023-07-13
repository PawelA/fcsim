c_cxx_flags = -Iinclude -MMD -O2
CFLAGS   = $(c_cxx_flags)
CXXFLAGS = $(c_cxx_flags)

LDLIBS = -lGL -lglfw

obj_fcsim = \
	fcsim/fcsim.o \
	fcsim/parse.o \
	fcsim/sincos.o \
	fcsim/strtod.o \
	fcsim/yxml/yxml.o \
	fcsim/box2d/Source/Collision/b2BroadPhase.o \
	fcsim/box2d/Source/Collision/b2CollideCircle.o \
	fcsim/box2d/Source/Collision/b2CollidePoly.o \
	fcsim/box2d/Source/Collision/b2Distance.o \
	fcsim/box2d/Source/Collision/b2PairManager.o \
	fcsim/box2d/Source/Collision/b2Shape.o \
	fcsim/box2d/Source/Common/b2BlockAllocator.o \
	fcsim/box2d/Source/Common/b2Settings.o \
	fcsim/box2d/Source/Common/b2StackAllocator.o \
	fcsim/box2d/Source/Dynamics/b2Body.o \
	fcsim/box2d/Source/Dynamics/b2ContactManager.o \
	fcsim/box2d/Source/Dynamics/b2Island.o \
	fcsim/box2d/Source/Dynamics/b2WorldCallbacks.o \
	fcsim/box2d/Source/Dynamics/b2World.o \
	fcsim/box2d/Source/Dynamics/Contacts/b2CircleContact.o \
	fcsim/box2d/Source/Dynamics/Contacts/b2Conservative.o \
	fcsim/box2d/Source/Dynamics/Contacts/b2Contact.o \
	fcsim/box2d/Source/Dynamics/Contacts/b2ContactSolver.o \
	fcsim/box2d/Source/Dynamics/Contacts/b2PolyAndCircleContact.o \
	fcsim/box2d/Source/Dynamics/Contacts/b2PolyContact.o \
	fcsim/box2d/Source/Dynamics/Joints/b2DistanceJoint.o \
	fcsim/box2d/Source/Dynamics/Joints/b2GearJoint.o \
	fcsim/box2d/Source/Dynamics/Joints/b2Joint.o \
	fcsim/box2d/Source/Dynamics/Joints/b2MouseJoint.o \
	fcsim/box2d/Source/Dynamics/Joints/b2PrismaticJoint.o \
	fcsim/box2d/Source/Dynamics/Joints/b2PulleyJoint.o \
	fcsim/box2d/Source/Dynamics/Joints/b2RevoluteJoint.o

obj_demo = \
	demo_src/demo.o

obj_all = $(obj_fcsim) $(obj_demo)
dep_all = $(obj_all:%.o=%.d)

libfcsim.a: $(obj_fcsim)
	ar rc $@ $^

demo: $(obj_demo) libfcsim.a
	$(CXX) $(LDFLAGS) -o $@ $^ $(LDLIBS)

clean:
	rm -f libfcsim.a demo
	rm -f $(obj_all)
	rm -f $(dep_all)

-include $(dep_all)
