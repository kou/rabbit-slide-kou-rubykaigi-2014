PROGRAMS =					\
	api

benchmark: $(PROGRAMS)
	./api $(DB_PATH)
	./binding-api.rb $(DB_PATH)
	./http-client.rb
	./binding.rb $(DB_PATH)
	./http-client.rb --n-times 8
	./binding.rb --n-times 8 $(DB_PATH)
	./http-client.rb --n-times 16
	./binding.rb --n-times 16 $(DB_PATH)

api: api.c
	cc -Wall -g3 $$(pkg-config --cflags --libs glib-2.0 groonga) -o $@ $<

