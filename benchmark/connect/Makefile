PROGRAMS =					\
	bench-epoll				\
	client-goroutine			\
	client-no-goroutine			\
	client-thread				\
	client-no-thread			\
	server-thread				\
	server-no-thread

all: $(PROGRAMS)

bench-epoll: bench-epoll.c
	cc -Wall -g3 $$(pkg-config --cflags --libs glib-2.0) -o $@ $<

client-goroutine: client-goroutine.go
	go build $<

client-no-goroutine: client-no-goroutine.go
	go build $<

client-thread: client-thread.c
	cc -Wall -g3 $$(pkg-config --cflags --libs gthread-2.0) -o $@ $<

client-no-thread: client-no-thread.c
	cc -Wall -g3 $$(pkg-config --cflags --libs glib-2.0) -o $@ $<

server-thread: server-thread.c
	cc -Wall -g3 $$(pkg-config --cflags --libs gthread-2.0) -o $@ $<

server-no-thread: server-no-thread.c
	cc -Wall -g3 $$(pkg-config --cflags --libs glib-2.0) -o $@ $<

clean:
	rm -f $(PROGRAMS)
