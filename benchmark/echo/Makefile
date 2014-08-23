PROGRAMS =					\
	bench-epoll				\
	client-goroutine			\
	client-epoll				\
	client-thread				\
	server-epoll				\
	server-thread

all: $(PROGRAMS)

bench-epoll: bench-epoll.c
	cc -Wall -g3 $$(pkg-config --cflags --libs gthread-2.0) -o $@ $<

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

large-message.json: message.json
	ruby -r json \
	  -e 'puts JSON.pretty_generate({"messages" => [JSON.parse(ARGF.read)] * 20000})' \
	  $< > $@

clean:
	rm -f $(PROGRAMS)
