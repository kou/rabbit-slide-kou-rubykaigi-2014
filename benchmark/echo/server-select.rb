#!/usr/bin/env ruby

require "optparse"
require "ostruct"
require "socket"
require "io/nonblock"

options = OpenStruct.new
options.port = 22929
parser = OptionParser.new
parser.on("--port=PORT", Integer,
          "(#{options.port})") do |port|
  options.port = port
end
parser.parse!

buffer_size = 8192

server = TCPServer.new(nil, options.port)
readers = [server]
writers = []
clients = []
write_buffers = {}

close_client = lambda do |client|
  client.close
  readers.delete(client)
  writers.delete(client)
  clients.delete(client)
  write_buffers.delete(client)
end

loop do
  readables, writables, exceptions = IO.select(readers, writers, clients)
  readables.each do |readable|
    if readable == server
      client = server.accept
      client.nonblock = true
      write_buffers[client] = []
      readers << client
      clients << client
    else
      client = readable
      begin
        data = client.readpartial(buffer_size)
      rescue EOFError, SystemCallError
        close_client.call(client)
      else
        buffers = write_buffers[client]
        writers << client if buffers.empty?
        buffers << data
      end
    end
  end

  writables.each do |client|
    next if client.closed?
    buffers = write_buffers[client]
    buffer = buffers.shift
    begin
      written_size = client.write(buffer)
    rescue SystemCallError
      close_client.call(client)
    else
      if written_size < buffer.bytesize
        buffers.unshift(buffer[written_size..-1])
      end
      writers.delete(client) if buffers.empty?
    end
  end

  exceptions.each do |client|
    close_client.call(client)
  end
end
