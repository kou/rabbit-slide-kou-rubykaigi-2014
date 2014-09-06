#!/usr/bin/env ruby

require "optparse"

buffer_size = 4096
parser = OptionParser.new
parser.on("--buffer-size=SIZE", Integer,
          "Buffer size",
          "(#{buffer_size})") do |size|
  buffer_size = size
end
parser.parse!

start = Time.now
ARGV.each do |path|
  File.open(path, "r") do |file|
    buffer = ""
    while file.read(buffer_size, buffer)
      print(buffer)
    end
  end
end
$stderr.puts(Time.now - start)
