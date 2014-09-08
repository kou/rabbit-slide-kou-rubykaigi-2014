#!/usr/bin/env ruby

text = File.read(ARGV[0])
if text.encoding == Encoding::ASCII_8BIT
  text.force_encoding("ASCII").scrub!
end

start = Time.now
tokens = []
text.each_char.each_cons(2) do |characters|
  tokens << characters.join("")
end
$stderr.puts(Time.now - start)
