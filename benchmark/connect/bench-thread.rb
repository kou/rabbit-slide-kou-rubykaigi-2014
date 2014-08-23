#!/usr/bin/env ruby

require "optparse"
require "ostruct"
require "thread"
require "socket"

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

Thread.abort_on_exception = true

statistics = Statistics.new
timer_thread = Thread.new do
  loop do
    sleep 1
    print("%0.3f connections/s\n" % statistics.throughput)
    statistics = Statistics.new
  end
end

options.n_concurrent_connections.times do
  Thread.new do
    loop do
      socket = TCPSocket.new(options.host, options.port)
      socket.close
      statistics.n_connections += 1
    end
  end
end

timer_thread.join
