#!/usr/bin/env ruby

require "optparse"
require "ostruct"
require "socket"

options = OpenStruct.new
options.host = "127.0.0.1"
options.port = 2929
parser = OtionParser.new
parser.on("--host=HOST",
          "(#{options.host})") do |host|
  options.host = host
end
parser.on("--port=PORT", Integer,
          "(#{options.port})") do |port|
  options.port = port
end
parser.parse!

server = TCPServer.new(options.host, options.port)
loop do
  Thread.new(server.accept) do |client|
    data = client.read(1)
    client.write(data)
    client.close
  end
end
