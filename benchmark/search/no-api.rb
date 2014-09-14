#!/usr/bin/env ruby

require "groonga"

Groonga::Database.open(ARGV[0])
entries = Groonga["Entries"]

start_time = Time.now
entries.find_all do |record|
  /Ruby/ =~ record.description
end
puts(Time.now - start_time)
