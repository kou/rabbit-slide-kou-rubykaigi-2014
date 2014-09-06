#!/usr/bin/env ruby

require "optparse"

n = 1
parser = OptionParser.new
parser.on("--n-times=N", Integer,
          "Search N times",
          "(#{n})") do |_n|
  n = _n
end
parser.parse!

require "groonga"

Groonga::Database.open(ARGV[0])
entries = Groonga["Entries"]

start_time = Time.now
n.times do
  matched_records = entries.select do |record|
    record.description =~ "文字列"
  end
  sorted_records = matched_records.sort([["_score", :desc]], :limit => 10)
  sorted_records.each do |record|
    [record.score, record.label, record.version, record.description]
  end
  matched_records.group("version").each do |grouped_record|
    [grouped_record._key, grouped_record.n_sub_records]
  end
end
puts(Time.now - start_time)
