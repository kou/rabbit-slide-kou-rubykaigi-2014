PROGRAMS =					\
	client-goroutine			\
	client-thread				\
	server-thread

all: $(PROGRAMS)

client-goroutine: client-goroutine.go
	go build $<

client-thread: client-thread.c
	cc -Wall -g3 $$(pkg-config --cflags --libs gthread-2.0) -o $@ $<

server-thread: server-thread.c
	cc -Wall -g3 $$(pkg-config --cflags --libs gthread-2.0) -o $@ $<

clean:
	rm -f $(PROGRAMS)
