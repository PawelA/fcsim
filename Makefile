.POSIX:
.SUFFIXES: .c .o .wasm

OBJ_COMMON = \
	src/arena.o \
	src/button.o \
	src/core.o \
	src/export.o \
	src/gen.o \
	src/graph.o \
	src/str.o \
	src/text.o \
	src/xml.o \
	src/box2d/b2Body.o \
	src/box2d/b2BroadPhase.o \
	src/box2d/b2CircleContact.o \
	src/box2d/b2CollideCircle.o \
	src/box2d/b2CollidePoly.o \
	src/box2d/b2Contact.o \
	src/box2d/b2ContactManager.o \
	src/box2d/b2ContactSolver.o \
	src/box2d/b2Island.o \
	src/box2d/b2PairManager.o \
	src/box2d/b2PolyAndCircleContact.o \
	src/box2d/b2PolyContact.o \
	src/box2d/b2RevoluteJoint.o \
	src/box2d/b2Shape.o \
	src/box2d/b2World.o \
	src/fpmath/atan2.o \
	src/fpmath/sincos.o \
	src/fpmath/strtod.o

OBJ = $(OBJ_COMMON) src/main.o

OBJ_WASM = $(OBJ_COMMON:.o=.wasm) \
	src/arch/wasm/gl.wasm \
	src/arch/wasm/malloc.wasm \
	src/arch/wasm/math.wasm \
	src/arch/wasm/string.wasm

HDR = \
	src/arena.h \
	src/button.h \
	src/gl.h \
	src/graph.h \
	src/interval.h \
	src/poocs.h \
	src/str.h \
	src/text.h \
	src/xml.h \
	include/box2d/b2Body.h \
	include/box2d/b2BroadPhase.h \
	include/box2d/b2CircleContact.h \
	include/box2d/b2CMath.h \
	include/box2d/b2Collision.h \
	include/box2d/b2Contact.h \
	include/box2d/b2ContactManager.h \
	include/box2d/b2ContactSolver.h \
	include/box2d/b2Island.h \
	include/box2d/b2NullContact.h \
	include/box2d/b2PairManager.h \
	include/box2d/b2PolyAndCircleContact.h \
	include/box2d/b2PolyContact.h \
	include/box2d/b2RevoluteJoint.h \
	include/box2d/b2Settings.h \
	include/box2d/b2Shape.h \
	include/box2d/b2Vec.h \
	include/box2d/b2World.h \
	include/fpmath/fpmath.h

HDR_WASM = \
	arch/wasm/include/stdlib.h \
	arch/wasm/include/math.h \
	arch/wasm/include/string.h

fcsim: $(OBJ)
	$(CC) -o $@ $^ -lm -lX11 -lGL

fcsim.wasm: $(OBJ_WASM)
	wasm-ld --no-entry --export-all --allow-undefined -o $@ $^

.c.o:
	$(CC) $(CFLAGS) -Iinclude -c -o $@ $<

.c.wasm:
	clang $(CFLAGS) -Iinclude -Iarch/wasm/include --target=wasm32 -nostdlib -c -o $@ $<

$(OBJ): $(HDR)
$(OBJ_WASM): $(HDR) $(HDR_WASM)

clean:
	rm -f fcsim fcsim.wasm $(OBJ) $(OBJ_WASM)
