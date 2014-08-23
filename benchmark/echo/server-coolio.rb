#!/usr/bin/env ruby

require "optparse"
require "ostruct"
require "coolio"
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

loop = Coolio::Loop.default
server = Coolio::TCPServer.new(nil, options.port) do |client|
  client.on_read do |data|
    case options.json_parser
    when :json
      JSON.parse(data)
    when :yajl
      Yajl::Parser.parse(data)
    when :oj
      Oj.load(data)
    end
    write(data)
  end
end
loop.attach(server)

loop.run
