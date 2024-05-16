cc      ?= cc
cxx     ?= c++
cflags  ?= -O2
ldlibs  ?= -lGL -lX11

cflags += -MMD -Iinclude

obj = \
	obj/arena_layer.o \
	obj/core.o \
	obj/conv.o \
	obj/generate.o \
	obj/main.o \
	obj/runner.o \
	obj/timer.o \
	obj/xml.o \
	obj/box2d/b2BlockAllocator.o \
	obj/box2d/b2Body.o \
	obj/box2d/b2BroadPhase.o \
	obj/box2d/b2CircleContact.o \
	obj/box2d/b2CollideCircle.o \
	obj/box2d/b2CollidePoly.o \
	obj/box2d/b2Contact.o \
	obj/box2d/b2ContactManager.o \
	obj/box2d/b2ContactSolver.o \
	obj/box2d/b2Distance.o \
	obj/box2d/b2Island.o \
	obj/box2d/b2Joint.o \
	obj/box2d/b2PairManager.o \
	obj/box2d/b2PolyAndCircleContact.o \
	obj/box2d/b2PolyContact.o \
	obj/box2d/b2RevoluteJoint.o \
	obj/box2d/b2Settings.o \
	obj/box2d/b2Shape.o \
	obj/box2d/b2StackAllocator.o \
	obj/box2d/b2World.o \
	obj/fpmath/atan2.o \
	obj/fpmath/sincos.o \
	obj/fpmath/strtod.o

objdir = \
	obj/box2d \
	obj/fpmath

fcsim: $(obj)
	$(cxx) -o $@ $^ $(ldlibs)

obj/%.o: src/%.c | $(objdir)
	$(cc) $(cflags) -c -o $@ $<

obj/%.o: src/%.cpp | $(objdir)
	$(cxx) $(cflags) -c -o $@ $<

$(objdir):
	mkdir -p $@

clean:
	rm -f fcsim
	rm -rf obj

-include $(obj:%.o=%.d)
