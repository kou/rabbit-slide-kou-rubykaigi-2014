#!/usr/bin/env ruby

require "optparse"
require "ostruct"
require "coolio"

options = OpenStruct.new
options.host = "127.0.0.1"
options.port = 2929
options.n_concurrent_connections = 10000
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
parser.parse!

class Statistics
  attr_accessor :n_connections
  def initialize
    clear
  end

  def clear
    @start = Time.now
    @n_connections = 0
  end

  def elapsed_time
    Time.now - @start
  end

  def throughput
    @n_connections / elapsed_time
  end
end

loop = Coolio::Loop.default

statistics = Statistics.new
timer = Coolio::TimerWatcher.new(1, true)
timer.on_timer do
  print("%0.3f connections/s\n" % statistics.throughput)
  statistics.clear
end
loop.attach(timer)

def connect(host, port, loop, statistics)
  socket = Coolio::TCPSocket.connect(host, port)
  socket.on_connect do
    close
  end
  socket.on_close do
    statistics.n_connections += 1
    connect(host, port, loop, statistics)
  end
  loop.attach(socket)
end

options.n_concurrent_connections.times do
  connect(options.host, options.port, loop, statistics)
end

loop.run
