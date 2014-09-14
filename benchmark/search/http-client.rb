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

require "net/http"
require "json"
require "rack/utils"

def search
  Net::HTTP.start("localhost", 10041) do |http|
    parameters = {
      "table"          => "Entries",
      "match_columns"  => "description",
      "query"          => "Ruby",
      "sortby"         => "-_score",
      "limit"          => 10,
      "output_columns" => "label,version,description",
      "drilldown"      => "version",
      "cache"          => "no",
    }
    path = "/d/select?#{Rack::Utils.build_query(parameters)}"
    response = http.get(path)
    JSON.parse(response.body)
  end
end

start_time = Time.now
if n == 1
  search
else
  threads = n.times.collect do
    Thread.new do
      search
    end
  end
  threads.each(&:join)
end
puts(Time.now - start_time)
