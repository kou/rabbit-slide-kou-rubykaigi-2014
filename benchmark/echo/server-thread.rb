#!/usr/bin/env ruby

require "optparse"
require "ostruct"
require "socket"
require "oj"
require "yajl"
require "json"

options = OpenStruct.new
options.port = 22929
options.json_parser = nil
parser = OptionParser.new
parser.on("--port=PORT", Integer,
          "(#{options.port})") do |port|
  options.port = port
end
json_parsers = [:json, :yajl, :oj]
parser.on("--json-parser=PARSER", json_parsers,
          "available parsers: #{json_parsers.inspect}",
          "(#{options.json_parser})") do |json_parser|
  options.json_parser = json_parser
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
        case options.json_parser
        when :json
          JSON.parse(buffer)
        when :yajl
          Yajl::Parser.parse(buffer)
        when :oj
          Oj.load(buffer)
        end
        client.write(buffer)
        client.flush
      end
    end
  end
end
