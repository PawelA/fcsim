include config.mk
include box2d.mk

FCSIM_OBJS = \
	fcsim/fcsim.o \
	fcsim/parse.o \
	fcsim/math.o \
	fcsim/yxml/yxml.o \
	$(BOX2D_OBJS) \
	net.o

DEMO_OBJS = \
	demo/main.o \
	demo/draw.o

demo/main: $(DEMO_OBJS) fcsim/fcsim.a
	$(CXX) -o $@ $^ $(LDFLAGS)

fcsim/fcsim.a: $(FCSIM_OBJS)
	ar rc $@ $^

config.mk: config.def.mk
	cp $< $@

clean:
	rm -f demo/main fcsim/fcsim.a $(FCSIM_OBJS) $(DEMO_OBJS)
