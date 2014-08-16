#!/usr/bin/env ruby

require "optparse"
require "ostruct"
require "socket"
require "thread"

options = OpenStruct.new
options.host = "127.0.0.1"
options.port = 2929
options.n_requests  = 40000
options.concurrency =  1000
parser = OptionParser.new
parser.on("--host=HOST",
          "(#{options.host})") do |host|
  options.host = host
end
parser.on("--port=PORT", Integer,
          "(#{options.port})") do |port|
  options.port = port
end
parser.on("--n-requests=N", Integer,
          "(#{options.n_requests})") do |n|
  options.n_requests = n
end
parser.on("--concurrency=N", Integer,
          "(#{options.concurrency})") do |n|
  options.concurrency = n
end
parser.parse!

messages = Queue.new
workers = options.concurrency.times.collect do
  Thread.new do
    loop do
      message = messages.pop
      break if message.nil?
      client = TCPSocket.new(options.host, options.port)
      client.write(message)
      client.read(1)
      client.close
    end
  end
end

start = Time.now
message = "x"
options.n_requests.times do |i|
  messages << message
end

options.concurrency.times do
  messages << nil
end

workers.each(&:join)
elapsed_time = Time.now - start

puts("Total:   %0.3fs" % elapsed_time)
puts("Average: %0.3fms" % ((elapsed_time / options.n_requests) * 1_000))
