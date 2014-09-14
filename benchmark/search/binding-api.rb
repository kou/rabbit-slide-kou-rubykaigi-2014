#!/usr/bin/env ruby

require "groonga"

Groonga::Database.open(ARGV[0])
entries = Groonga["Entries"]

start_time = Time.now
entries.select do |record|
  record.description =~ "Ruby"
end
puts(Time.now - start_time)
