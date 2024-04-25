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

all: demo$(exe)

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

### glfw

obj-glfw-y += obj/glfw/src/context.o
obj-glfw-y += obj/glfw/src/egl_context.o
obj-glfw-y += obj/glfw/src/init.o
obj-glfw-y += obj/glfw/src/input.o
obj-glfw-y += obj/glfw/src/monitor.o
obj-glfw-y += obj/glfw/src/osmesa_context.o
obj-glfw-y += obj/glfw/src/vulkan.o
obj-glfw-y += obj/glfw/src/window.o

obj-glfw-$(win32) += obj/glfw/src/wgl_context.o
obj-glfw-$(win32) += obj/glfw/src/win32_init.o
obj-glfw-$(win32) += obj/glfw/src/win32_joystick.o
obj-glfw-$(win32) += obj/glfw/src/win32_monitor.o
obj-glfw-$(win32) += obj/glfw/src/win32_thread.o
obj-glfw-$(win32) += obj/glfw/src/win32_time.o
obj-glfw-$(win32) += obj/glfw/src/win32_window.o

obj-glfw-$(linux) += obj/glfw/src/linux_joystick.o
obj-glfw-$(linux) += obj/glfw/src/posix_thread.o
obj-glfw-$(linux) += obj/glfw/src/posix_time.o
obj-glfw-$(linux) += obj/glfw/src/xkb_unicode.o

obj-glfw-$(x11) += obj/glfw/src/glx_context.o
obj-glfw-$(x11) += obj/glfw/src/x11_init.o
obj-glfw-$(x11) += obj/glfw/src/x11_monitor.o
obj-glfw-$(x11) += obj/glfw/src/x11_window.o

obj-glfw-$(wayland) += obj/glfw/src/wl_init.o
obj-glfw-$(wayland) += obj/glfw/src/wl_monitor.o
obj-glfw-$(wayland) += obj/glfw/src/wl_window.o

obj-glfw-n += obj/glfw/src/cocoa_time.o
obj-glfw-n += obj/glfw/src/null_init.o
obj-glfw-n += obj/glfw/src/null_joystick.o
obj-glfw-n += obj/glfw/src/null_monitor.o
obj-glfw-n += obj/glfw/src/null_window.o

include glfw-wayland.mk

obj-y += $(obj-glfw-y)
obj-n += $(obj-glfw-n)

$(obj-glfw-y): cflags-y += -Iglfw/include
$(obj-glfw-y): cflags-$(win32)   += -D_GLFW_WIN32
$(obj-glfw-y): cflags-$(x11)     += -D_GLFW_X11
$(obj-glfw-y): cflags-$(wayland) += -D_GLFW_WAYLAND
$(obj-glfw-y): | obj/glfw/src
obj/glfw/src:
	mkdir -p obj/glfw/src

### demo

obj-demo-y = \
	obj/src/arena_layer.o \
	obj/src/conv.o \
	obj/src/demo.o \
	obj/src/generate.o \
	obj/src/runner.o \
	obj/src/text.o \
	obj/src/timer.o \
	obj/src/xml.o

obj-y += $(obj-demo-y)
obj-n += $(obj-demo-n)

$(obj-demo-y): cflags-y += -Iglfw/include -Ibox2d/include -Ifpmath/include
$(obj-demo-y): | obj/src
obj/src:
	mkdir -p obj/src

### rules

obj/%.o: %.c
	$(cc) -MMD $(cflags-y) -c -o $@ $<

obj/%.o: %.cpp
	$(cxx) -MMD $(cflags-y) -c -o $@ $<

demo$(exe): $(obj-y)
	$(cxx) -o $@ $^ $(ldlibs-y)

clean:
	rm -f demo$(exe)
	rm -rf obj

-include $(obj-y:%.o=%.d)
