#!/usr/bin/env ruby

require "optparse"
require "ostruct"
require "coolio"

MESSAGE = <<-MESSAGE
{
  "name": "Alice",
  "nick": "alice",
  "age": 14
}
MESSAGE

options = OpenStruct.new
options.host = "127.0.0.1"
options.port = 22929
options.n_requests  = 1000
options.concurrency = 500
options.n_messages  =  100
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
parser.on("--n-messages=N", Integer,
          "(#{options.n_messages})") do |n|
  options.n_messages = n
end
parser.parse!

def connect(loop, options)
  socket = Coolio::TCPSocket.connect(options.host, options.port)
  n_rest_messages = options.n_messages
  socket.write(MESSAGE)
  socket.on_read do |data|
    n_rest_messages -= 1
    if n_rest_messages > 0
      write(MESSAGE)
    else
      close
      yield
    end
  end
  loop.attach(socket)
end

loop = Coolio::Loop.default
n_rest_requests = options.n_requests
options.concurrency.times.each do
  on_finished = lambda do
    n_rest_requests -= 1
    connect(loop, options, &on_finished) if n_rest_requests > 0
  end
  connect(loop, options, &on_finished)
end

start = Time.now
loop.run
elapsed_time = Time.now - start

puts("Total:   %0.3fs" % elapsed_time)
puts("Average: %0.3fms" % ((elapsed_time / options.n_requests) * 1_000))
