#!/usr/bin/env ruby

require "optparse"
require "ostruct"
require "socket"

options = OpenStruct.new
options.port = 2929
options.n_workers = 8
parser = OptionParser.new
parser.on("--port=PORT", Integer,
          "(#{options.port})") do |port|
  options.port = port
end
parser.on("--n-workers=N", Integer,
          "(#{options.n_workers})") do |n|
  options.n_workers = n
end
parser.parse!

server = TCPServer.new(nil, options.port)

workers = []
options.n_workers.times do
  notify_read, notify_write = IO.pipe
  pid = fork do
    Process.setsid

    notify_write.close
    catch do |tag|
      loop do
        readables, = IO.select([server, notify_read])
        readables.each do |readable|
          if readable == notify_read
            throw(tag)
          else
            client = readable.accept
            client.close
          end
        end
      end
    end
  end
  notify_read.close
  workers << {
    :pid => pid,
    :notify_pipe => notify_write,
  }
end

trap(:INT) do
  workers.each do |worker|
    worker[:notify_pipe].write("close")
    worker[:notify_pipe].flush
  end
end

workers.each do |worker|
  Process.waitpid(worker[:pid])
end
