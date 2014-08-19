#!/usr/bin/env ruby

require "optparse"
require "ostruct"
require "socket"

options = OpenStruct.new
options.port = 22929
parser = OptionParser.new
parser.on("--port=PORT", Integer,
          "(#{options.port})") do |port|
  options.port = port
end
parser.parse!

server = TCPServer.new(options.port)
loop do
  Thread.new(server.accept) do |client|
    buffer = ""
    buffer_size = 4096
    loop do
      begin
        client.readpartial(buffer_size, buffer)
      rescue EOFEror
        break
      else
        client.write(buffer)
        client.flush
      end
    end
  end
end
