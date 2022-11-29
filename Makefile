include config.mk
include box2d.mk

OBJS = \
	draw.o \
	fcsim.o \
	fcparse.o \
	main.o \
	net.o

$(TARGET): $(OBJS) box2d.a yxml.a
	$(CXX) -o $@ $^ $(LDFLAGS)

draw.o:  fcsim.h
fcsim.o: fcsim.h net.h $(BOX2D_HDRS)
fcparse.o: fcsim.h
main.o:  fcsim.h draw.h net.h

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
