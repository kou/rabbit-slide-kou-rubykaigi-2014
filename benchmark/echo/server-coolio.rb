#!/usr/bin/env ruby

require "optparse"
require "ostruct"
require "coolio"
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

loop = Coolio::Loop.default
server = Coolio::TCPServer.new(nil, options.port) do |client|
  client.on_read do |data|
    if options.parse_data
      if data[0] == "{"
        case options.json_parser
        when :json
          JSON.parse(data)
        when :yajl
          Yajl::Parser.parse(data)
        when :oj
          Oj.load(data)
        end
      else
        MessagePack.unpack(data)
      end
    end

    write(data)
  end
end
loop.attach(server)

loop.run
