/*!
 * common.js (https://github.com/SamsungDForum/NativePlayer)
 * Copyright 2016, Samsung Electronics Co., Ltd
 * Licensed under the MIT license
 *
 * @file common.js
 * @brief Common NaCl application page configuration.
 */

var logs;
var logs_level;
if (typeof logs_level === "undefined") {
  logs_level = "log";
}
var kLogPrefix = "LOG:";
var kErrorPrefix = "ERROR:";
var kDebugPrefix = "DEBUG:";

function updateStatus(message) {
  document.getElementById("status").innerHTML = message;
}

/**
 * Return true when |s| starts with the string |prefix|.
 *
 * @param {string} s The string to search.
 * @param {string} prefix The prefix to search for in |s|.
 */
function startsWith(s, prefix) {
  return s.indexOf(prefix) === 0;
}

/**
 * Print message in the logs area if it has a specific prefix (defined in
 * kLogPrefix or kErrorPrefix).
 * @param message - a string message
 * @returns true - if this message was a log or error message.
 */
function printIfLog(message) {
  if ((typeof message == "string") && (uses_logging == true) &&
          (startsWith(message, kLogPrefix) || startsWith(message, kErrorPrefix)
           || startsWith(message, kDebugPrefix))) {
    console.log(message);
    logs.value += message;
    logs.scrollTop = logs.scrollHeight;
    return true;
  }
  return false;
}

function handleNaclCrash(event) {
  var nacl_module = document.getElementById("nacl_module");
  updateStatus("Crashed/exited with status: " + nacl_module.exitStatus);
}

function handleNaclLoad(event) {
  updateStatus("Loaded successfully.");
  if (typeof(exampleSpecificActionAfterNaclLoad) == "function") {
    exampleSpecificActionAfterNaclLoad();
  }
}

/**
 * Creates a NaCl module with params from example.js and attach listeners.
 *
 * @param nmf_path_name - complete manifest file path with name
 * @param width - of a NaCl module"s view in pixels
 * @param height - of a NaCl module"s view in pixels
 */
function createNaclModule(nmf_path_name, width, height) {
  var nacl_elem = document.createElement("embed");
  nacl_elem.setAttribute("name", "nacl_module");
  nacl_elem.setAttribute("id", "nacl_module");
  nacl_elem.setAttribute("type", "application/x-nacl");
  nacl_elem.setAttribute("src", nmf_path_name);
  nacl_elem.setAttribute("width", width);
  nacl_elem.setAttribute("height", height);
  nacl_elem.setAttribute("logs", logs_level);

  var listenerDiv = document.getElementById("listener");
  listenerDiv.appendChild(nacl_elem);

  // attach common listeners
  listenerDiv.addEventListener("message", handleNaclMessage, true);
  listenerDiv.addEventListener("crash", handleNaclCrash, true);
  listenerDiv.addEventListener("load", handleNaclLoad, true);
}

/**
 * Configure this page with example specific elements when document finishes
 * loading.
 */
document.addEventListener("DOMContentLoaded", function() {
  document.getElementById("page_title").innerHTML = demo_name;
  document.getElementById("demo_header").innerHTML += demo_name;
  document.getElementById("description").innerHTML = demo_description;
  logs = document.getElementById("logs");
  if (uses_logging == false) {
    document.getElementById("logs_area").style.display = "none";
  }

  createNaclModule(nmf_path_name, nacl_width, nacl_height);
  updateStatus("Loading...");
});
