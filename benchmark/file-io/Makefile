PROGRAMS =					\
	cat-buffer				\
	cat-no-buffer

all: $(PROGRAMS)

cat-buffer: cat-buffer.c
	cc -Wall -O3 -g3 $$(pkg-config --cflags --libs glib-2.0) -o $@ $<

cat-no-buffer: cat-no-buffer.c
	cc -Wall -O3 -g3 $$(pkg-config --cflags --libs glib-2.0) -o $@ $<

benchmark: $(PROGRAMS)
	dd if=/dev/zero of=/tmp/file bs=1024 count=200000
	cat /tmp/file > /dev/null
	bash -c 'time cat /tmp/file > /dev/null'
	./cat-buffer --buffer-size 1024 /tmp/file > /dev/null
	./cat-buffer --buffer-size 4096 /tmp/file > /dev/null
	./cat-buffer --buffer-size 8192 /tmp/file > /dev/null
	./cat-buffer --buffer-size 16384 /tmp/file > /dev/null
	./cat-no-buffer --chunk-size 1024 /tmp/file > /dev/null
	./cat-no-buffer --chunk-size 4096 /tmp/file > /dev/null
	./cat-no-buffer --chunk-size 8192 /tmp/file > /dev/null
	./cat-no-buffer --chunk-size 16384 /tmp/file > /dev/null
	./cat-buffer.rb --buffer-size 1024 /tmp/file > /dev/null
	./cat-buffer.rb --buffer-size 4096 /tmp/file > /dev/null
	./cat-buffer.rb --buffer-size 8192 /tmp/file > /dev/null
	./cat-buffer.rb --buffer-size 16384 /tmp/file > /dev/null
	./cat-no-buffer.rb --chunk-size 1024 /tmp/file > /dev/null
	./cat-no-buffer.rb --chunk-size 4096 /tmp/file > /dev/null
	./cat-no-buffer.rb --chunk-size 8192 /tmp/file > /dev/null
	./cat-no-buffer.rb --chunk-size 16384 /tmp/file > /dev/null

clean:
	rm -f $(PROGRAMS)
