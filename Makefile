cc      ?= cc
cxx     ?= c++
cflags  ?= -O2

os_Windows_NT = n
os_$(OS) = y

win32   ?= $(os_Windows_NT)
x11     ?= y
wayland ?= n

y-or-y = y
n-or-y = y
y-or-n = y
n-or-n = n

linux = $($(x11)-or-$(wayland))

any = $($(win32)-or-$(linux))

exe-win32-n =
exe-win32-y = .exe
exe = $(exe-win32-$(win32))

### common

all: fcsim$(exe)

cflags-y = $(cflags)
ldlibs-y =
ldlibs-$(win32)   += -lgdi32 -lopengl32 -lws2_32 -mwindows
ldlibs-$(linux)   += -lGL
ldlibs-$(x11)     += -lX11
ldlibs-$(wayland) += -lwayland-client

### fpmath

obj-fpmath-y = \
	obj/fpmath/atan2.o \
	obj/fpmath/sincos.o \
	obj/fpmath/strtod.o

obj-y += $(obj-fpmath-y)
obj-n += $(obj-fpmath-n)

$(obj-fpmath-y): cflags-y += -Ifpmath/include
$(obj-fpmath-y): | obj/fpmath
obj/fpmath:
	mkdir -p obj/fpmath

### box2d

obj-box2d-y = \
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

obj-y += $(obj-box2d-y)
obj-n += $(obj-box2d-n)

$(obj-box2d-y): cflags-y += -Ibox2d/include -Ifpmath/include
$(obj-box2d-y): | obj/box2d/src
obj/box2d/src:
	mkdir -p obj/box2d/src

### main

obj-main-y = \
	obj/src/arena_layer.o \
	obj/src/core.o \
	obj/src/conv.o \
	obj/src/generate.o \
	obj/src/main.o \
	obj/src/runner.o \
	obj/src/timer.o \
	obj/src/xml.o

obj-y += $(obj-main-y)
obj-n += $(obj-main-n)

$(obj-main-y): cflags-y += -Ibox2d/include -Ifpmath/include
$(obj-main-y): | obj/src
obj/src:
	mkdir -p obj/src

### rules

obj/%.o: %.c
	$(cc) -MMD $(cflags-y) -c -o $@ $<

obj/%.o: %.cpp
	$(cxx) -MMD $(cflags-y) -c -o $@ $<

fcsim$(exe): $(obj-y)
	$(cxx) -o $@ $^ $(ldlibs-y)

clean:
	rm -f fcsim$(exe)
	rm -rf obj

-include $(obj-y:%.o=%.d)
