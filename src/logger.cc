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

#include <stdio.h>
#include <stdarg.h>
#include <string>

#include "ppapi/cpp/instance.h"
#include "ppapi/cpp/var.h"


pp::Instance* Logger::instance_ = NULL;
bool Logger::show_debug_ = false;

const char* kLogPrefix = "LOG: ";
const char* kDebugPrefix = "DEBUG: ";
const char* kErrorPrefix = "ERROR: ";
const unsigned kMaxMessageSize = 256;

void Logger::InitializeInstance(pp::Instance* instance) {
  if (!instance_)
    instance_ = instance;
}

void Logger::Log(const std::string& message) {
  InternalPrint(kLogPrefix, message);
}

void Logger::Log(const char* message_format, ...) {
  va_list arguments_list;
  va_start(arguments_list, message_format);
  InternalPrint(kLogPrefix, message_format, arguments_list);
  va_end(arguments_list);
}

void Logger::Log(int line, const char* func,const char* file,
    const char* message_format, ...)  {
  va_list arguments_list;
  va_start(arguments_list, message_format);
  InternalPrint(line, func, file, kLogPrefix, message_format, arguments_list);
  va_end(arguments_list);
}

void Logger::Error(const std::string& message) {
  InternalPrint(kErrorPrefix, message);
}

void Logger::Error(const char* message_format, ...) {
  va_list arguments_list;
  va_start(arguments_list, message_format);
  InternalPrint(kErrorPrefix, message_format, arguments_list);
  va_end(arguments_list);
}

void Logger::Error(int line, const char* func,const char* file,
    const char* message_format, ...)  {
  va_list arguments_list;
  va_start(arguments_list, message_format);
  InternalPrint(line, func, file, kErrorPrefix, message_format, arguments_list);
  va_end(arguments_list);
}

void Logger::Debug(const std::string& message) {
  if (!show_debug_)
    return;
  InternalPrint(kDebugPrefix, message);
}

void Logger::Debug(const char* message_format, ...) {
  if (!show_debug_)
    return;
  va_list arguments_list;
  va_start(arguments_list, message_format);
  InternalPrint(kDebugPrefix, message_format, arguments_list);
  va_end(arguments_list);
}

void Logger::Debug(int line, const char* func,const char* file,
    const char* message_format, ...)  {
  if (!show_debug_)
    return;
  va_list arguments_list;
  va_start(arguments_list, message_format);
  InternalPrint(line, func, file, kDebugPrefix, message_format, arguments_list);
  va_end(arguments_list);
}

void Logger::EnableDebugLogs(bool flag) {
  show_debug_ = flag;
}

void Logger::InternalPrint(const char* prefix, const std::string& message) {
  if (instance_)
    instance_->PostMessage(prefix + message + "\n");
  if (prefix == kLogPrefix)
    LOG_POINT("%s", message.c_str());
  else if (prefix == kDebugPrefix)
    DEBUG_POINT("%s", message.c_str());
  if (prefix == kErrorPrefix)
    ERROR_POINT("%s", message.c_str());
}

void Logger::InternalPrint(const char* prefix, const char* message_format,
                           va_list arguments_list) {
  if (instance_) {
    char buff[kMaxMessageSize];
    std::string log_format = std::string(prefix) + message_format + "\n";
    vsnprintf(buff, kMaxMessageSize, log_format.c_str(), arguments_list);
    instance_->PostMessage(buff);
    if (prefix == kLogPrefix)
      LOG_POINT("%s", buff);
    else if (prefix == kDebugPrefix)
      DEBUG_POINT("%s", buff);
    if (prefix == kErrorPrefix)
      ERROR_POINT("%s", buff);
  }
}

void Logger::InternalPrint(int line, const char* func, const char* file,
                          const char* prefix, const char* message_format,
                          va_list arguments_list) {
  if (instance_) {
    char buff[kMaxMessageSize];
    std::string log_format = std::string(prefix) + message_format + "\n";
    vsnprintf(buff, kMaxMessageSize, log_format.c_str(), arguments_list);
    instance_->PostMessage(buff);
    if (prefix == kLogPrefix)
      LOG_POINT("[%s/%s:%d] %s", file, func, line, buff);
    else if (prefix == kDebugPrefix)
      DEBUG_POINT("[%s/%s:%d] %s", file, func, line, buff);
    if (prefix == kErrorPrefix)
      ERROR_POINT("[%s/%s:%d] %s", file, func, line, buff);
  }
}
