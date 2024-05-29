wasm-cc = clang
wasm-cflags = -O2 -MMD -Iinclude -Iarch/wasm/include --target=wasm32 -nostdlib

wasm-cxx = clang++
wasm-cxxflags = -O2 -MMD -Iinclude -Iarch/wasm/include --target=wasm32 -nostdlib -fno-rtti -fno-exceptions

wasm-ld = wasm-ld
wasm-ldflags = --no-entry --export-all --allow-undefined

wasm-obj = \
	obj/wasm/arena_layer.o \
	obj/wasm/core.o \
	obj/wasm/conv.o \
	obj/wasm/generate.o \
	obj/wasm/xml.o \
	obj/wasm/arch/wasm/math.o \
	obj/wasm/arch/wasm/malloc.o \
	obj/wasm/arch/wasm/gl.o \
	obj/wasm/arch/wasm/string.o \
	obj/wasm/box2d/b2BlockAllocator.o \
	obj/wasm/box2d/b2Body.o \
	obj/wasm/box2d/b2BroadPhase.o \
	obj/wasm/box2d/b2CircleContact.o \
	obj/wasm/box2d/b2CollideCircle.o \
	obj/wasm/box2d/b2CollidePoly.o \
	obj/wasm/box2d/b2Contact.o \
	obj/wasm/box2d/b2ContactManager.o \
	obj/wasm/box2d/b2ContactSolver.o \
	obj/wasm/box2d/b2Distance.o \
	obj/wasm/box2d/b2Island.o \
	obj/wasm/box2d/b2Joint.o \
	obj/wasm/box2d/b2PairManager.o \
	obj/wasm/box2d/b2PolyAndCircleContact.o \
	obj/wasm/box2d/b2PolyContact.o \
	obj/wasm/box2d/b2RevoluteJoint.o \
	obj/wasm/box2d/b2Settings.o \
	obj/wasm/box2d/b2Shape.o \
	obj/wasm/box2d/b2StackAllocator.o \
	obj/wasm/box2d/b2World.o \
	obj/wasm/fpmath/atan2.o \
	obj/wasm/fpmath/sincos.o \
	obj/wasm/fpmath/strtod.o

wasm-objdir = \
	obj/wasm/box2d \
	obj/wasm/fpmath \
	obj/wasm/arch/wasm

html/fcsim.wasm: $(wasm-obj)
	$(wasm-ld) $(wasm-ldflags) -o $@ $^ $(wasm-ldlibs)

obj/wasm/%.o: src/%.c | $(wasm-objdir)
	$(wasm-cc) $(wasm-cflags) -c -o $@ $<

obj/wasm/%.o: src/%.cpp | $(wasm-objdir)
	$(wasm-cxx) $(wasm-cxxflags) -c -o $@ $<

$(wasm-objdir):
	mkdir -p $@

clean:
	rm -f html/fcsim.wasm
	rm -rf obj

-include $(obj:%.o=%.d)
