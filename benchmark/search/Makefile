benchmark:
	./http-client.rb
	./binding.rb $(DB_PATH)
	./http-client.rb --n-times 8
	./binding.rb --n-times 8 $(DB_PATH)
	./http-client.rb --n-times 16
	./binding.rb --n-times 16 $(DB_PATH)
