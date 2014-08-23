PROGRAMS =					\
	client-goroutine			\
	client-epoll				\
	client-thread				\
	server-epoll				\
	server-thread

all: $(PROGRAMS)

client-goroutine: client-goroutine.go
	go build $<

client-epoll: client-epoll.c
	cc -Wall -g3 $$(pkg-config --cflags --libs gthread-2.0) -o $@ $<

client-thread: client-thread.c
	cc -Wall -g3 $$(pkg-config --cflags --libs gthread-2.0) -o $@ $<

server-epoll: server-epoll.c
	cc -Wall -g3 \
	  $$(pkg-config --cflags --libs gthread-2.0 json-glib-1.0) \
	  -lmsgpack \
	  -o $@ $<

server-thread: server-thread.c
	cc -Wall -g3 \
	  $$(pkg-config --cflags --libs gthread-2.0 json-glib-1.0) \
	  -lmsgpack \
	  -o $@ $<

clean:
	rm -f $(PROGRAMS)
