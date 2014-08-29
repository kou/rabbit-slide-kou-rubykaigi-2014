#!/usr/bin/env ruby

start = Time.now
ARGV.each do |path|
  File.open(path, "r") do |file|
    IO.copy_stream(file, $stdout)
  end
end
$stderr.puts(Time.now - start)
