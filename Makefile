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

### fcsim

obj-fcsim-y = \
	fcsim/fcsim.o \
	fcsim/parse.o \
	fcsim/sincos.o \
	fcsim/atan2.o \
	fcsim/conv.o \
	fcsim/generate.o \
	fcsim/strtod.o \
	fcsim/strtoi.o \
	fcsim/xml.o \
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

obj-y += $(obj-fcsim-y)
obj-n += $(obj-fcsim-n)

$(obj-fcsim-y): cflags-y += -Iinclude

### glfw

obj-glfw-y += stuff/glfw/src/context.o
obj-glfw-y += stuff/glfw/src/egl_context.o
obj-glfw-y += stuff/glfw/src/init.o
obj-glfw-y += stuff/glfw/src/input.o
obj-glfw-y += stuff/glfw/src/monitor.o
obj-glfw-y += stuff/glfw/src/osmesa_context.o
obj-glfw-y += stuff/glfw/src/vulkan.o
obj-glfw-y += stuff/glfw/src/window.o

obj-glfw-$(win32) += stuff/glfw/src/wgl_context.o
obj-glfw-$(win32) += stuff/glfw/src/win32_init.o
obj-glfw-$(win32) += stuff/glfw/src/win32_joystick.o
obj-glfw-$(win32) += stuff/glfw/src/win32_monitor.o
obj-glfw-$(win32) += stuff/glfw/src/win32_thread.o
obj-glfw-$(win32) += stuff/glfw/src/win32_time.o
obj-glfw-$(win32) += stuff/glfw/src/win32_window.o

obj-glfw-$(linux) += stuff/glfw/src/linux_joystick.o
obj-glfw-$(linux) += stuff/glfw/src/posix_thread.o
obj-glfw-$(linux) += stuff/glfw/src/posix_time.o
obj-glfw-$(linux) += stuff/glfw/src/xkb_unicode.o

obj-glfw-$(x11) += stuff/glfw/src/glx_context.o
obj-glfw-$(x11) += stuff/glfw/src/x11_init.o
obj-glfw-$(x11) += stuff/glfw/src/x11_monitor.o
obj-glfw-$(x11) += stuff/glfw/src/x11_window.o

obj-glfw-$(wayland) += stuff/glfw/src/wl_init.o
obj-glfw-$(wayland) += stuff/glfw/src/wl_monitor.o
obj-glfw-$(wayland) += stuff/glfw/src/wl_window.o

obj-glfw-n += stuff/glfw/src/cocoa_time.o
obj-glfw-n += stuff/glfw/src/null_init.o
obj-glfw-n += stuff/glfw/src/null_joystick.o
obj-glfw-n += stuff/glfw/src/null_monitor.o
obj-glfw-n += stuff/glfw/src/null_window.o

include glfw-wayland.mk

obj-y += $(obj-glfw-y)
obj-n += $(obj-glfw-n)

$(obj-glfw-y): cflags-y += -Istuff/glfw/include
$(obj-glfw-y): cflags-$(win32)   += -D_GLFW_WIN32
$(obj-glfw-y): cflags-$(x11)     += -D_GLFW_X11
$(obj-glfw-y): cflags-$(wayland) += -D_GLFW_WAYLAND

### demo

obj-demo-y = \
	stuff/arena_layer.o \
	stuff/demo.o \
	stuff/file.o \
	stuff/http.o \
	stuff/load_layer.o \
	stuff/loader.o \
	stuff/runner.o \
	stuff/text.o \
	stuff/timer.o \
	stuff/ui.o

obj-y += $(obj-demo-y)
obj-n += $(obj-demo-n)

$(obj-demo-y): cflags-y += -Iinclude -Istuff/glfw/include

### rules

%.o: %.c
	$(cc) -MMD $(cflags-y) -c -o $@ $<

%.o: %.cpp
	$(cxx) -MMD $(cflags-y) -c -o $@ $<

demo$(exe): $(obj-y)
	$(cxx) -o $@ $^ $(ldlibs-y)

obj = $(obj-y) $(obj-n)
dep = $(obj:%.o=%.d)

clean:
	rm -f demo demo.exe
	rm -f $(obj)
	rm -f $(dep)
	rm -f $(clean)

-include $(obj-y:%.o=%.d)
