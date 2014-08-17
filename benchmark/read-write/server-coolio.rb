#!/usr/bin/env ruby

require "optparse"
require "ostruct"
require "coolio"

options = OpenStruct.new
options.port = 2929
parser = OptionParser.new
parser.on("--port=PORT", Integer,
          "(#{options.port})") do |port|
  options.port = port
end
parser.parse!

loop = Coolio::Loop.default
server = Coolio::TCPServer.new(nil, options.port) do |client|
  client.on_read do |data|
    write(data)
  end
  client.on_write_complete do
    close
  end
end
loop.attach(server)

loop.run
