#!/usr/bin/env node

var net = require("net");
var options = require("commander");

intOption = function(newValue, oldValue) {
  return parseInt(newValue);
}

options
  .option("--host <host>", "Host to connect", "127.0.0.1")
  .option("--port <port>", "Port number", intOption, 22929)
  .option("--n-requests <n>", "The number of requests.",             1000)
  .option("--concurrency <n>", "The number of workers",               500)
  .option("--n-messages <n>", "The number of messages in a session",  100)
  .parse(process.argv);

var message =
  "{\n" +
  "  \"name\": \"Alice\",\n" +
  "  \"nick\": \"alice\",\n" +
  "  \"age\": 14\n" +
  "}\n";
var nRestRequests = options.nRequests;
var nRestWorkers = options.concurrency;
var startTime = null;

function runWorker() {
  nRestRequests--;
  var nRestMessages = options.nMessages;
  var connectOptions = {
    host: options.host,
    port: options.port
  };
  var client = net.connect(connectOptions,
			   function() {
			     client.write(message);
			   });
  client.on("data", function(data) {
    nRestMessages--;
    if (nRestMessages > 0) {
      client.write(message);
    } else {
      client.end();
    }
  });
  client.on("end", function() {
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
