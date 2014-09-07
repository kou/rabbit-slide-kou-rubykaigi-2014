#!/usr/bin/env ruby

text = File.read("ruby.txt")

start = Time.now
tokens = []
(text.size - 1).times do |i|
  tokens << text[i, 2]
end
$stderr.puts(Time.now - start)
