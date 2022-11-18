include config.mk
include box2d.mk

OBJS = \
	draw.o \
	fcsim.o \
	main.o

$(TARGET): $(OBJS) box2d.a
	$(CXX) -o $@ $^ $(LDFLAGS)

draw.o:  fcsim.h
fcsim.o: fcsim.h $(BOX2D_HDRS)
main.o:  fcsim.h draw.h level.h

box2d.a: $(BOX2D_OBJS)
	ar rc $@ $^

$(BOX2D_OBJS): $(BOX2D_HDRS)

parse: parse.o yxml.a

yxml/yxml.o: yxml/yxml.c
	$(CC) -I yxml -c -o $@ $^

yxml.a: yxml/yxml.o
	ar rc $@ $^

config.mk: config.def.mk
	cp $< $@

clean:
	rm -f $(TARGET) box2d.a $(OBJS) $(BOX2D_OBJS)
