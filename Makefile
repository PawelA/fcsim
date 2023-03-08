include config.mk
include box2d.mk

OBJ = \
	fcsim/fcsim.o \
	fcsim/parse.o \
	fcsim/math.o \
	fcsim/yxml/yxml.o \
	fcsim/stdlib/math.o \
	fcsim/stdlib/new.o \
	fcsim/stdlib/stdlib.o \
	fcsim/stdlib/string.o \
	fcsim/stdlib/virtual.o \
	$(BOX2D_OBJS)

fcsim.wasm: $(OBJ)
	$(LD) $(LDFLAGS) -o $@ $^

config.mk: config.def.mk
	cp $< $@

clean:
	rm -f demo/main fcsim/fcsim.a $(FCSIM_OBJS) $(DEMO_OBJS)
