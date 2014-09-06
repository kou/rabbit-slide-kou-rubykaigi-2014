PROGRAMS =					\
	cat-buffer				\
	cat-no-buffer

all: $(PROGRAMS)

cat-buffer: cat-buffer.c
	cc -Wall -O3 -g3 $$(pkg-config --cflags --libs glib-2.0) -o $@ $<

cat-no-buffer: cat-no-buffer.c
	cc -Wall -O3 -g3 $$(pkg-config --cflags --libs glib-2.0) -o $@ $<

clean:
	rm -f $(PROGRAMS)
