#!/usr/bin/env ruby

require "optparse"
require "ostruct"
require "socket"

options = OpenStruct.new
options.port = 12929
parser = OptionParser.new
parser.on("--port=PORT", Integer,
          "(#{options.port})") do |port|
  options.port = port
end
parser.parse!

server = TCPServer.new(options.port)
loop do
  Thread.new(server.accept) do |client|
    data = client.read(1)
    client.write(data)
    client.close
  end
end
