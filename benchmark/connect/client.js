#!/usr/bin/env node

var net = require("net");
var options = require("commander");

intOption = function(newValue, oldValue) {
  return parseInt(newValue);
}

options
  .option("--host <host>", "Host to connect", "127.0.0.1")
  .option("--port <port>", "Port number", intOption, 2929)
  .option("--n-requests <n>", "The number of requests.", 30000)
  .option("--concurrency <n>", "The number of workers",   1000)
  .parse(process.argv);

var nRestRequests = options.nRequests;
var nRestWorkers = options.concurrency;
var startTime = null;

function runWorker() {
  var connectOptions = {
    host: options.host,
    port: options.port
  };
  var client = net.connect(connectOptions,
			   function() {
			     client.write("x");
			   });
  client.on("data", function(data) {
    client.end();
  });
  client.on("end", function() {
    nRestRequests--;
    if (nRestRequests > 0) {
      runWorker();
    } else {
      nRestWorkers--;
      if (nRestWorkers == 0) {
        var elapsedTimeMS = new Date() - startTime;
        console.log("Total:   " + (elapsedTimeMS / 1000) + "s");
        console.log("Average: " + (elapsedTimeMS / options.nRequests) + "ms");
      }
    }
  });
}

for (var i = 0; i < options.concurrency; i++) {
  runWorker();
}

startTime = new Date();
