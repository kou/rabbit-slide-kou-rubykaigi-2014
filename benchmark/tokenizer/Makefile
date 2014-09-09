benchmark: bigram ruby-ja.txt ruby-en.txt
	./bigram ruby-ja.txt
	go run bigram.go ruby-ja.txt
	./bigram-aref.rb ruby-ja.txt
	./bigram-each.rb ruby-ja.txt
	./bigram ruby-en.txt
	go run bigram.go ruby-en.txt
	./bigram-aref.rb ruby-en.txt
	./bigram-each.rb ruby-en.txt
	LANG=C ./bigram-aref.rb ruby-en.txt
	LANG=C ./bigram-each.rb ruby-en.txt

bigram: bigram.c
	cc -Wall -O3 -g3 $$(pkg-config --cflags --libs glib-2.0) -o $@ $<

ruby-ja.txt:
	curl -o $@ "http://ja.wikipedia.org/wiki/Ruby?action=raw"

ruby-en.txt:
	curl -o $@ "http://en.wikipedia.org/wiki/Ruby_(programming_language)?action=raw"
