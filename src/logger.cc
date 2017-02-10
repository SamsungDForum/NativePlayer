/*!
 * logger.cc (https://github.com/SamsungDForum/NativePlayer)
 * Copyright 2016, Samsung Electronics Co., Ltd
 * Licensed under the MIT license
 *
 * @author  Tomasz Borkowski
 *
 * @brief
 * This is a NaCl Logger, that sends text messages (and adds a specific prefix)
 * to JS side with use of pp::Instance.PostMessage() method.
 */

#include "logger.h"

#include <array>
#include <stdio.h>
#include <stdarg.h>
#include <string>
#include <libgen.h>

#include <chrono>

#include "ppapi/cpp/instance.h"
#include "ppapi/cpp/var.h"

pp::Instance* Logger::instance_ = NULL;
LogLevel Logger::js_log_level_ = LogLevel::kNone;
#ifdef DEBUG_LOGS
LogLevel Logger::std_log_level_ = LogLevel::kInfo;
#else
LogLevel Logger::std_log_level_ = LogLevel::kNone;
#endif

namespace {

const unsigned kMaxMessageSize = 256;

double GetTimestamp() {
  using std::chrono::steady_clock;
  using std::chrono::duration_cast;
  using std::chrono::duration;

  static auto st_begin = steady_clock::now();
  auto now = steady_clock::now();

  return duration<double>{now - st_begin}.count();
}

const std::array<std::string, 4> kLogPrefixes = {{
  "",         // LogLevel::kNone
  "ERROR: ",  // LogLevel::kError
  "INFO: ",   // LogLevel::kInfo
  "DEBUG: ",  // LogLevel::kDebug
}};

const std::array<std::string, 4> kLogLevelColors = {{
  "",          // LogLevel::kNone
  "\033[31m",  // LogLevel::kError
  "\033[32m",  // LogLevel::kInfo
  "\033[32m",  // LogLevel::kDebug
}};

}  // anonymous namespace

void Logger::InitializeInstance(pp::Instance* instance) {
  if (!instance_)
    instance_ = instance;
}

void Logger::Info(const std::string& message) {
  InternalPrint(LogLevel::kInfo, message);
}

void Logger::Info(const char* message_format, ...) {
  va_list arguments_list;
  va_start(arguments_list, message_format);
  InternalPrint(LogLevel::kInfo, nullptr, message_format, arguments_list);
  va_end(arguments_list);
}

void Logger::Info(int line, const char* func,const char* file,
    const char* message_format, ...)  {
  va_list arguments_list;
  va_start(arguments_list, message_format);
  InternalPrint(line, func, file, LogLevel::kInfo, message_format,
                arguments_list);
  va_end(arguments_list);
}

void Logger::Error(const std::string& message) {
  InternalPrint(LogLevel::kError, message);
}

void Logger::Error(const char* message_format, ...) {
  va_list arguments_list;
  va_start(arguments_list, message_format);
  InternalPrint(LogLevel::kError, nullptr, message_format, arguments_list);
  va_end(arguments_list);
}

void Logger::Error(int line, const char* func,const char* file,
    const char* message_format, ...)  {
  va_list arguments_list;
  va_start(arguments_list, message_format);
  InternalPrint(line, func, file, LogLevel::kError, message_format,
                arguments_list);
  va_end(arguments_list);
}

void Logger::Debug(const std::string& message) {
  InternalPrint(LogLevel::kDebug, message);
}

void Logger::Debug(const char* message_format, ...) {
  va_list arguments_list;
  va_start(arguments_list, message_format);
  InternalPrint(LogLevel::kDebug, nullptr, message_format, arguments_list);
  va_end(arguments_list);
}

void Logger::Debug(int line, const char* func,const char* file,
    const char* message_format, ...)  {
  va_list arguments_list;
  va_start(arguments_list, message_format);
  InternalPrint(line, func, file, LogLevel::kDebug, message_format,
                arguments_list);
  va_end(arguments_list);
}

void Logger::SetJsLogLevel(LogLevel level) {
  js_log_level_ = level;
}

void Logger::SetStdLogLevel(LogLevel level) {
  std_log_level_ = level;
}

void Logger::InternalPrint(LogLevel level, const char* std_prefix,
                           const char* message) {
  if (instance_ && level <= js_log_level_)
    instance_->PostMessage(
        kLogPrefixes[static_cast<int>(level)] + message + "\n");
  if (level > std_log_level_)
    return;
  printf("%s %11.6f %s%s%s\033[0m\n",
         kLogLevelColors[static_cast<int>(level)].c_str(),
         GetTimestamp(),
         std_prefix ? std_prefix : "",
         kLogPrefixes[static_cast<int>(level)].c_str(),
         message);
  fflush(stdout);
}

void Logger::InternalPrint(LogLevel level, const char* std_prefix,
                           const char* message_format, va_list arguments_list) {
  if (!IsLoggingEnabled())
    return;
  char buff[kMaxMessageSize];
  vsnprintf(buff, kMaxMessageSize, message_format, arguments_list);
  InternalPrint(level, std_prefix, buff);
}

void Logger::InternalPrint(int line, const char* func, const char* file,
                          LogLevel level, const char* message_format,
                          va_list arguments_list) {
  if (!IsLoggingEnabled())
    return;
  char buff[kMaxMessageSize];
  const char* file_basename = basename(const_cast<char*>(file));
  snprintf(buff, kMaxMessageSize, "[%s/%s:%d] ", file_basename, func, line);
  InternalPrint(level, buff, message_format, arguments_list);
}
