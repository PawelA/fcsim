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
	fcsim/sincos.o \
	fcsim/atan2.o \
	fcsim/conv.o \
	fcsim/generate.o \
	fcsim/strtod.o \
	fcsim/strtoi.o \
	fcsim/xml.o

obj-y += $(obj-fcsim-y)
obj-n += $(obj-fcsim-n)

$(obj-fcsim-y): cflags-y += -Iinclude -Ibox2d/include

### box2d

obj-box2d-y = \
	box2d/src/Collision/b2BroadPhase.o \
	box2d/src/Collision/b2CollideCircle.o \
	box2d/src/Collision/b2CollidePoly.o \
	box2d/src/Collision/b2Distance.o \
	box2d/src/Collision/b2PairManager.o \
	box2d/src/Collision/b2Shape.o \
	box2d/src/Common/b2BlockAllocator.o \
	box2d/src/Common/b2Settings.o \
	box2d/src/Common/b2StackAllocator.o \
	box2d/src/Dynamics/b2Body.o \
	box2d/src/Dynamics/b2ContactManager.o \
	box2d/src/Dynamics/b2Island.o \
	box2d/src/Dynamics/b2World.o \
	box2d/src/Dynamics/Contacts/b2CircleContact.o \
	box2d/src/Dynamics/Contacts/b2Contact.o \
	box2d/src/Dynamics/Contacts/b2ContactSolver.o \
	box2d/src/Dynamics/Contacts/b2PolyAndCircleContact.o \
	box2d/src/Dynamics/Contacts/b2PolyContact.o \
	box2d/src/Dynamics/Joints/b2Joint.o \
	box2d/src/Dynamics/Joints/b2RevoluteJoint.o

obj-y += $(obj-box2d-y)
obj-n += $(obj-box2d-n)

$(obj-box2d-y): cflags-y += -Iinclude -Ibox2d/include

### glfw

obj-glfw-y += glfw/src/context.o
obj-glfw-y += glfw/src/egl_context.o
obj-glfw-y += glfw/src/init.o
obj-glfw-y += glfw/src/input.o
obj-glfw-y += glfw/src/monitor.o
obj-glfw-y += glfw/src/osmesa_context.o
obj-glfw-y += glfw/src/vulkan.o
obj-glfw-y += glfw/src/window.o

obj-glfw-$(win32) += glfw/src/wgl_context.o
obj-glfw-$(win32) += glfw/src/win32_init.o
obj-glfw-$(win32) += glfw/src/win32_joystick.o
obj-glfw-$(win32) += glfw/src/win32_monitor.o
obj-glfw-$(win32) += glfw/src/win32_thread.o
obj-glfw-$(win32) += glfw/src/win32_time.o
obj-glfw-$(win32) += glfw/src/win32_window.o

obj-glfw-$(linux) += glfw/src/linux_joystick.o
obj-glfw-$(linux) += glfw/src/posix_thread.o
obj-glfw-$(linux) += glfw/src/posix_time.o
obj-glfw-$(linux) += glfw/src/xkb_unicode.o

obj-glfw-$(x11) += glfw/src/glx_context.o
obj-glfw-$(x11) += glfw/src/x11_init.o
obj-glfw-$(x11) += glfw/src/x11_monitor.o
obj-glfw-$(x11) += glfw/src/x11_window.o

obj-glfw-$(wayland) += glfw/src/wl_init.o
obj-glfw-$(wayland) += glfw/src/wl_monitor.o
obj-glfw-$(wayland) += glfw/src/wl_window.o

obj-glfw-n += glfw/src/cocoa_time.o
obj-glfw-n += glfw/src/null_init.o
obj-glfw-n += glfw/src/null_joystick.o
obj-glfw-n += glfw/src/null_monitor.o
obj-glfw-n += glfw/src/null_window.o

include glfw-wayland.mk

obj-y += $(obj-glfw-y)
obj-n += $(obj-glfw-n)

$(obj-glfw-y): cflags-y += -Iglfw/include
$(obj-glfw-y): cflags-$(win32)   += -D_GLFW_WIN32
$(obj-glfw-y): cflags-$(x11)     += -D_GLFW_X11
$(obj-glfw-y): cflags-$(wayland) += -D_GLFW_WAYLAND

### demo

obj-demo-y = \
	src/arena_layer.o \
	src/demo.o \
	src/file.o \
	src/http.o \
	src/load_layer.o \
	src/loader.o \
	src/runner.o \
	src/text.o \
	src/timer.o \
	src/ui.o

obj-y += $(obj-demo-y)
obj-n += $(obj-demo-n)

$(obj-demo-y): cflags-y += -Iinclude -Iglfw/include

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
