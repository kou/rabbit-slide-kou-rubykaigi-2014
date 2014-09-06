#!/usr/bin/env ruby

require "optparse"

chunk_size = 4096
parser = OptionParser.new
parser.on("--chunk-size=SIZE", Integer,
          "Chunk size",
          "(#{chunk_size})") do |size|
  chunk_size = size
end
parser.parse!

start = Time.now
ARGV.each do |path|
  File.open(path, "r") do |file|
    loop do
      chunk = file.read(chunk_size)
      break if chunk.nil?
      print(chunk)
    end
  end
end
$stderr.puts(Time.now - start)
