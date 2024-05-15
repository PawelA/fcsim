cc      ?= cc
cxx     ?= c++
cflags  ?= -O2
ldlibs  ?= -lGL -lX11

all: fcsim

### fpmath

obj-fpmath = \
	obj/fpmath/atan2.o \
	obj/fpmath/sincos.o \
	obj/fpmath/strtod.o

obj += $(obj-fpmath)

$(obj-fpmath): cflags += -Ifpmath/include
$(obj-fpmath): | obj/fpmath
obj/fpmath:
	mkdir -p obj/fpmath

### box2d

obj-box2d = \
	obj/box2d/src/b2BlockAllocator.o \
	obj/box2d/src/b2Body.o \
	obj/box2d/src/b2BroadPhase.o \
	obj/box2d/src/b2CircleContact.o \
	obj/box2d/src/b2CollideCircle.o \
	obj/box2d/src/b2CollidePoly.o \
	obj/box2d/src/b2Contact.o \
	obj/box2d/src/b2ContactManager.o \
	obj/box2d/src/b2ContactSolver.o \
	obj/box2d/src/b2Distance.o \
	obj/box2d/src/b2Island.o \
	obj/box2d/src/b2Joint.o \
	obj/box2d/src/b2PairManager.o \
	obj/box2d/src/b2PolyAndCircleContact.o \
	obj/box2d/src/b2PolyContact.o \
	obj/box2d/src/b2RevoluteJoint.o \
	obj/box2d/src/b2Settings.o \
	obj/box2d/src/b2Shape.o \
	obj/box2d/src/b2StackAllocator.o \
	obj/box2d/src/b2World.o

obj += $(obj-box2d)

$(obj-box2d): cflags += -Ibox2d/include -Ifpmath/include
$(obj-box2d): | obj/box2d/src
obj/box2d/src:
	mkdir -p obj/box2d/src

### main

obj-main = \
	obj/src/arena_layer.o \
	obj/src/core.o \
	obj/src/conv.o \
	obj/src/generate.o \
	obj/src/main.o \
	obj/src/runner.o \
	obj/src/timer.o \
	obj/src/xml.o

obj += $(obj-main)

$(obj-main): cflags += -Ibox2d/include -Ifpmath/include
$(obj-main): | obj/src
obj/src:
	mkdir -p obj/src

### rules

obj/%.o: %.c
	$(cc) -MMD $(cflags) -c -o $@ $<

obj/%.o: %.cpp
	$(cxx) -MMD $(cflags) -c -o $@ $<

fcsim: $(obj)
	$(cxx) -o $@ $^ $(ldlibs)

clean:
	rm -f fcsim
	rm -rf obj

-include $(obj:%.o=%.d)
