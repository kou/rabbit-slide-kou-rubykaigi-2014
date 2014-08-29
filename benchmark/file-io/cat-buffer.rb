#!/usr/bin/env ruby

start = Time.now
ARGV.each do |path|
  File.open(path, "r") do |file|
    buffer_size = 8192
    buffer = ""
    while file.read(buffer_size, buffer)
      print(buffer)
    end
  end
end
$stderr.puts(Time.now - start)
