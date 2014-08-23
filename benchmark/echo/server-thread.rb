#!/usr/bin/env ruby

require "optparse"
require "ostruct"
require "socket"
require "oj"
require "yajl"
require "json"
require "msgpack"

options = OpenStruct.new
options.port = 22929
options.parse_data = false
options.json_parser = nil
parser = OptionParser.new
parser.on("--port=PORT", Integer,
          "(#{options.port})") do |port|
  options.port = port
end
parser.on("--[no-]parse-data",
          "(#{options.parse_data})") do |boolean|
  options.parse_data = boolean
end
json_parsers = [:json, :yajl, :oj]
parser.on("--json-parser=PARSER", json_parsers,
          "available parsers: #{json_parsers.inspect}",
          "(#{options.json_parser})") do |json_parser|
  options.parse_data = true
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
      end

      if options.parse_data
        if buffer[0] == "{"
          case options.json_parser
          when :json
            JSON.parse(buffer)
          when :yajl
            Yajl::Parser.parse(buffer)
          when :oj
            Oj.load(buffer)
          end
        else
          MessagePack.unpack(buffer)
        end
      end

      client.write(buffer)
      client.flush
    end
  end
end
