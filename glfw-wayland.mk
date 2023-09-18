# location of the wayland-protocols directory
wayland-protocols = /usr/share/wayland-protocols

### headers

wayland-protocol-headers = \
	stuff/glfw/src/wayland-xdg-shell-client-protocol.h \
	stuff/glfw/src/wayland-xdg-decoration-client-protocol.h \
	stuff/glfw/src/wayland-viewporter-client-protocol.h \
	stuff/glfw/src/wayland-relative-pointer-unstable-v1-client-protocol.h \
	stuff/glfw/src/wayland-pointer-constraints-unstable-v1-client-protocol.h \
	stuff/glfw/src/wayland-idle-inhibit-unstable-v1-client-protocol.h

# make needs to know about these before CC can generate dependencies
$(obj-glfw-$(wayland)): $(wayland-protocol-headers)

clean += $(wayland-protocol-headers)

stuff/glfw/src/wayland-xdg-shell-client-protocol.h: 
	wayland-scanner client-header \
	  $(wayland-protocols)/stable/xdg-shell/xdg-shell.xml \
	  $@

stuff/glfw/src/wayland-xdg-decoration-client-protocol.h:
	wayland-scanner client-header \
	  $(wayland-protocols)/unstable/xdg-decoration/xdg-decoration-unstable-v1.xml \
	  $@

stuff/glfw/src/wayland-viewporter-client-protocol.h:
	wayland-scanner client-header \
	  $(wayland-protocols)/stable/viewporter/viewporter.xml \
	  $@

stuff/glfw/src/wayland-relative-pointer-unstable-v1-client-protocol.h:
	wayland-scanner client-header \
	  $(wayland-protocols)/unstable/relative-pointer/relative-pointer-unstable-v1.xml \
	  $@

stuff/glfw/src/wayland-pointer-constraints-unstable-v1-client-protocol.h:
	wayland-scanner client-header \
	  $(wayland-protocols)/unstable/pointer-constraints/pointer-constraints-unstable-v1.xml \
	  $@

stuff/glfw/src/wayland-idle-inhibit-unstable-v1-client-protocol.h:
	wayland-scanner client-header \
	  $(wayland-protocols)/unstable/idle-inhibit/idle-inhibit-unstable-v1.xml \
	  $@

### code

wayland-protocol-sources = \
	stuff/glfw/src/wayland-xdg-shell-client-protocol.c \
	stuff/glfw/src/wayland-xdg-decoration-client-protocol.c \
	stuff/glfw/src/wayland-viewporter-client-protocol.c \
	stuff/glfw/src/wayland-relative-pointer-unstable-v1-client-protocol.c \
	stuff/glfw/src/wayland-pointer-constraints-unstable-v1-client-protocol.c \
	stuff/glfw/src/wayland-idle-inhibit-unstable-v1-client-protocol.c

obj-glfw-$(wayland) += $(wayland-protocol-sources:%.c=%.o)

clean += $(wayland-protocol-sources)

stuff/glfw/src/wayland-xdg-shell-client-protocol.o: stuff/glfw/src/wayland-xdg-shell-client-protocol.c
stuff/glfw/src/wayland-xdg-shell-client-protocol.c:
	wayland-scanner public-code \
	  $(wayland-protocols)/stable/xdg-shell/xdg-shell.xml \
	  $@

stuff/glfw/src/wayland-xdg-decoration-client-protocol.o: stuff/glfw/src/wayland-xdg-decoration-client-protocol.c
stuff/glfw/src/wayland-xdg-decoration-client-protocol.c:
	wayland-scanner public-code \
	  $(wayland-protocols)/unstable/xdg-decoration/xdg-decoration-unstable-v1.xml \
	  $@

stuff/glfw/src/wayland-viewporter-client-protocol.o: stuff/glfw/src/wayland-viewporter-client-protocol.c
stuff/glfw/src/wayland-viewporter-client-protocol.c:
	wayland-scanner public-code \
	  $(wayland-protocols)/stable/viewporter/viewporter.xml \
	  $@

stuff/glfw/src/wayland-relative-pointer-unstable-v1-client-protocol.o: stuff/glfw/src/wayland-relative-pointer-unstable-v1-client-protocol.c
stuff/glfw/src/wayland-relative-pointer-unstable-v1-client-protocol.c:
	wayland-scanner public-code \
	  $(wayland-protocols)/unstable/relative-pointer/relative-pointer-unstable-v1.xml \
	  $@

stuff/glfw/src/wayland-pointer-constraints-unstable-v1-client-protocol.o: stuff/glfw/src/wayland-pointer-constraints-unstable-v1-client-protocol.c
stuff/glfw/src/wayland-pointer-constraints-unstable-v1-client-protocol.c:
	wayland-scanner public-code \
	  $(wayland-protocols)/unstable/pointer-constraints/pointer-constraints-unstable-v1.xml \
	  $@

stuff/glfw/src/wayland-idle-inhibit-unstable-v1-client-protocol.o: stuff/glfw/src/wayland-idle-inhibit-unstable-v1-client-protocol.c
stuff/glfw/src/wayland-idle-inhibit-unstable-v1-client-protocol.c:
	wayland-scanner public-code \
	  $(wayland-protocols)/unstable/idle-inhibit/idle-inhibit-unstable-v1.xml \
	  $@
