#!/usr/bin/env ruby

require "optparse"
require "ostruct"
require "socket"
require "thread"

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
options.concurrency =  500
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

jobs = Queue.new
workers = options.concurrency.times.collect do
  Thread.new do
    loop do
      job = jobs.pop
      break if job.nil?
      client = TCPSocket.new(options.host, options.port)
      options.n_messages.times do
        written_size = client.write(MESSAGE)
        client.flush
        client.read(written_size)
      end
      client.close
    end
  end
end

start = Time.now
options.n_requests.times do |i|
  jobs << true
end
options.concurrency.times do
  jobs << nil
end
workers.each(&:join)
elapsed_time = Time.now - start

puts("Total:   %0.3fs" % elapsed_time)
puts("Average: %0.3fms" % ((elapsed_time / options.n_requests) * 1_000))
