benchmark: ruby-ja.txt ruby-en.txt
	go run bigram.go ruby-ja.txt
	./bigram-aref.rb ruby-ja.txt
	./bigram-each.rb ruby-ja.txt
	go run bigram.go ruby-en.txt
	./bigram-aref.rb ruby-en.txt
	./bigram-each.rb ruby-en.txt
	LANG=C ./bigram-aref.rb ruby-en.txt
	LANG=C ./bigram-each.rb ruby-en.txt

ruby-ja.txt:
	curl -o $@ "http://ja.wikipedia.org/wiki/Ruby?action=raw"

ruby-en.txt:
	curl -o $@ "http://en.wikipedia.org/wiki/Ruby_(programming_language)?action=raw"
