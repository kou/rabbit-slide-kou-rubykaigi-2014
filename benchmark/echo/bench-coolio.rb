#!/usr/bin/env ruby

require "English"
require "optparse"
require "ostruct"
require "coolio"

options = OpenStruct.new
options.host = "127.0.0.1"
options.port = 22929
options.n_concurrent_connections = 10000
options.message_size = 1024
parser = OptionParser.new
parser.on("--host=HOST",
          "(#{options.host})") do |host|
  options.host = host
end
parser.on("--port=PORT", Integer,
          "(#{options.port})") do |port|
  options.port = port
end
parser.on("--n-concurrent-connections=N", Integer,
          "(#{options.n_concurrent_connections})") do |n|
  options.n_concurrent_connections = n
end
parser.on("--message-size=SIZE",
          "(#{options.message_size})") do |size|
  case size
  when /MB?\z/i
    size = (Float($PREMATCH) * 1024 * 1024).round
  when /KB?\z/i
    size = (Float($PREMATCH) * 1024).round
  when /B?\z/i
    size = Integer($PREMATCH)
  end
  options.message_size = size
end
parser.parse!

class Statistics
  attr_accessor :n_finished_messages
  def initialize
    @start = Time.now
    @n_finished_messages = 0
  end

  def elapsed_time
    Time.now - @start
  end

  def throughput
    @n_finished_messages / elapsed_time
  end
end

loop = Coolio::Loop.default

statistics = Statistics.new
timer = Coolio::TimerWatcher.new(1, true)
timer.on_timer do
  print("%0.3f messages/s\n" % statistics.throughput)
  statistics = Statistics.new
end
loop.attach(timer)

message = "X" * options.message_size
options.n_concurrent_connections.times do
  socket = Coolio::TCPSocket.connect(options.host, options.port)
  socket.write(message)
  read_size = 0
  socket.on_read do |data|
    read_size += data.bytesize
    if read_size == message.bytesize
      statistics.n_finished_messages += 1
      socket.write(message)
      read_size = 0
    end
  end
  loop.attach(socket)
end

loop.run
