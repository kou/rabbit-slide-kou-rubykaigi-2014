#!/usr/bin/env ruby

require "optparse"
require "ostruct"
require "coolio"

options = OpenStruct.new
options.port = 22929
options.concurrency = 8
parser = OptionParser.new
parser.on("--port=PORT", Integer,
          "(#{options.port})") do |port|
  options.port = port
end
parser.on("--concurrency=N", Integer,
          "(#{options.concurrency})") do |concurrency|
  options.concurrency = concurrency
end
parser.parse!

raw_server = TCPServer.new(nil, options.port)

workers = []
options.concurrency.times do
  notify_read, notify_write = IO.pipe
  pid = fork do
    Process.setsid
    loop = Coolio::Loop.default

    server = Coolio::TCPServer.new(raw_server) do |client|
      client.on_read do |data|
        write(data)
      end
    end
    loop.attach(server)

    notify_write.close
    notify_io = Coolio::IO.new(notify_read)
    notify_io.on_read do
      server.close
      close
    end
    loop.attach(notify_io)

    loop.run
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
  end
end

workers.each do |worker|
  Process.waitpid(worker[:pid])
end
