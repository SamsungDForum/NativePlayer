/*!
 * logger.h (https://github.com/SamsungDForum/NativePlayer)
 * Copyright 2016, Samsung Electronics Co., Ltd
 * Licensed under the MIT license
 *
 * @author  Tomasz Borkowski
 *
 * @brief
 * This is a NaCl Logger that sends text messages (and adds a specific prefix)
 * to JS side with use of pp::Instance.PostMessage() method.
 */

#ifndef COMMON_SRC_LOGGER_H_
#define COMMON_SRC_LOGGER_H_

#include <stdarg.h>
#include <string>

#include "ppapi/cpp/instance.h"

#ifdef DEBUG_LOGS
#define LOG_POINT(format, ...) do { \
  printf("\033[32m" format "\033[0m", \
   ##__VA_ARGS__); \
  fflush(stdout); \
} while (0)
#define DEBUG_POINT(format, ...) do { \
  printf("\033[33m" format "\033[0m", \
  ##__VA_ARGS__); \
  fflush(stdout); \
} while (0)
#define ERROR_POINT(format, ...) do { \
  printf("\033[31m" format "\033[0m", \
  ##__VA_ARGS__); \
  fflush(stdout); \
} while (0)
#else
#define LOG_POINT(format, ...) /* */
#define DEBUG_POINT(format, ...) /* */
#define ERROR_POINT(format, ...) /* */
#endif

/**
 * Utility class that simplifies sending log messages by PostMessage to JS.
 */
class Logger {
 public:
  /**
   * Initializes the pp::Instance pointer, so that the Logger could post
   * messages to JS.
   */
  static void InitializeInstance(pp::Instance* instance);

  /**
   * Adds a log prefix and a newline character at the end to the passed string
   * and sends it to JS.
   */
  static void Log(const std::string& message);

  /**
   * Does the same as log(const std::string&), but takes arguments like standard
   * stdio printf() function.
   * \link http://www.cplusplus.com/reference/cstdio/printf/
   */
  static void Log(const char* message_format, ...);

  /**
   * Does the same as Log(const char* message_format, ...),
   * but takes also arguments to provide info from where it was called
   */
  static void Log(int line, const char* func,const char* file,
      const char* message_format, ...);
  /**
   * Does the same as log( ), but adds error prefix.
   */
  static void Error(const std::string& message);
  static void Error(const char* message_format, ...);
  static void Error(int line, const char* func,const char* file,
      const char* message_format, ...);

  /**
   * Similar to Error and Log, but with debug prefix. These logs are disabled
   * by default. <code>EnableDebugLogs(true)</code> must be called to show these
   * logs.
   */
  static void Debug(const std::string& message);
  static void Debug(const char* message_format, ...);
  static void Debug(int line, const char* func,const char* file,
      const char* message_format, ...);

  /**
   * Enables logs sent by <code>Debug</code> methods if passed <code>flag</code>
   * is true. Disables debug logs when <code>flag</code> is false.
   * By default debug logs are disabled.
   */
  static void EnableDebugLogs(bool flag);

 private:
  /**
   * Internal wrappers for printing to PostMessage with specified prefix.
   */
  static void InternalPrint(const char* prefix, const std::string& message);
  static void InternalPrint(const char* prefix, const char* message_format,
                            va_list arguments_list);
  static void InternalPrint(int line, const char* func, const char* file,
                            const char* prefix, const char* message_format,
                            va_list arguments_list);

  static pp::Instance* instance_;
  static bool show_debug_;
};

#endif  // COMMON_SRC_LOGGER_H_
