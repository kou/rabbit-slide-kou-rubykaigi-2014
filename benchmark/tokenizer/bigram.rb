#!/usr/bin/env ruby

text = File.read("ruby.txt")

start = Time.now
tokens = []
text.each_char.each_cons(2) do |characters|
  tokens << characters.join("")
end
$stderr.puts(Time.now - start)
