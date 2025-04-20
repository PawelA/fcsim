DIR = box2d/ fpmath/ arch/wasm/

SRC_CORE = \
	arena \
	button \
	export \
	core \
	gen \
	graph \
	str \
	text \
	xml

SRC_BOX2D = \
	box2d/b2BlockAllocator \
	box2d/b2Body \
	box2d/b2BroadPhase \
	box2d/b2CircleContact \
	box2d/b2CollideCircle \
	box2d/b2CollidePoly \
	box2d/b2Contact \
	box2d/b2ContactManager \
	box2d/b2ContactSolver \
	box2d/b2Island \
	box2d/b2PairManager \
	box2d/b2PolyAndCircleContact \
	box2d/b2PolyContact \
	box2d/b2RevoluteJoint \
	box2d/b2Settings \
	box2d/b2Shape \
	box2d/b2StackAllocator \
	box2d/b2World

SRC_FPMATH = \
	fpmath/atan2 \
	fpmath/sincos \
	fpmath/strtod

SRC_ARCH_WASM = \
	arch/wasm/math \
	arch/wasm/malloc \
	arch/wasm/gl \
	arch/wasm/string

# linux

SRC_LINUX = $(SRC_CORE) $(SRC_BOX2D) $(SRC_FPMATH) main
OBJ_LINUX = $(SRC_LINUX:%=obj/linux/%.o)

fcsim: $(OBJ_LINUX)
	c++ -o $@ $^ -lX11 -lGL

obj/linux/%.o: src/%.c
	cc -O2 -MMD -Iinclude -c -o $@ $<

obj/linux/%.o: src/%.cpp
	c++ -O2 -MMD -Iinclude -c -o $@ $<

# wasm

SRC_WASM = $(SRC_CORE) $(SRC_BOX2D) $(SRC_FPMATH) $(SRC_ARCH_WASM)
OBJ_WASM = $(SRC_WASM:%=obj/wasm/%.o)

html/fcsim.wasm: $(OBJ_WASM)
	wasm-ld --no-entry --export-all --allow-undefined -o $@ $^

obj/wasm/%.o: src/%.c
	clang -O2 -MMD -Iinclude -Iarch/wasm/include --target=wasm32 -nostdlib -c -o $@ $<

obj/wasm/%.o: src/%.cpp
	clang++ -O2 -MMD -Iinclude -Iarch/wasm/include --target=wasm32 -nostdlib -fno-rtti -fno-exceptions -c -o $@ $<

# misc

clean:
	rm -rf obj/
	rm -f fcsim html/fcsim.wasm

%/:
	mkdir -p $@

$(OBJ_LINUX): | $(DIR:%=obj/linux/%)
$(OBJ_WASM): | $(DIR:%=obj/wasm/%)

OBJ = $(OBJ_LINUX) $(OBJ_WASM)

-include $(OBJ:%.o=%.d)
