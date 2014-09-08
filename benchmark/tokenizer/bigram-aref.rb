#!/usr/bin/env ruby

text = File.read(ARGV[0])
if text.encoding == Encoding::ASCII_8BIT
  text.force_encoding("ASCII").scrub!
end

start = Time.now
tokens = []
(text.size - 1).times do |i|
  tokens << text[i, 2]
end
$stderr.puts(Time.now - start)
