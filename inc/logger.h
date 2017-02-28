/*!
 * logger.h (https://github.com/SamsungDForum/NativePlayer)
 * Copyright 2016, Samsung Electronics Co., Ltd
 * Licensed under the MIT license
 *
 * THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS "AS IS"
 * AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
 * IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE
 * ARE DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT HOLDER BE LIABLE FOR ANY
 * DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES
 * (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES;
 * LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND
 * ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT
 * (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF THIS
 * SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
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

enum class LogLevel {
  kNone = 0,
  kError,
  kInfo,
  kDebug,

  kMinLevel = kNone,
  kMaxLevel = kDebug,
};

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
   * Adds an info prefix and a newline character at the end to the passed string
   * and sends it to JS.
   */
  static void Info(const std::string& message);

  /**
   * Does the same as log(const std::string&), but takes arguments like standard
   * stdio printf() function.
   * \link http://www.cplusplus.com/reference/cstdio/printf/
   */
  static void Info(const char* message_format, ...);

  /**
   * Does the same as Log(const char* message_format, ...),
   * but also takes arguments to provide info from where it was called.
   */
  static void Info(int line, const char* func,const char* file,
      const char* message_format, ...);
  /**
   * Does the same as log( ), but adds error prefix.
   */
  static void Error(const std::string& message);
  static void Error(const char* message_format, ...);
  static void Error(int line, const char* func,const char* file,
      const char* message_format, ...);

  /**
   * Similar to Error and Log, but with debug prefix.
   */
  static void Debug(const std::string& message);
  static void Debug(const char* message_format, ...);
  static void Debug(int line, const char* func,const char* file,
      const char* message_format, ...);

  /**
   * Change a level of logs that will be sent to JS.
   */
  static void SetJsLogLevel(LogLevel level);

   /**
   * Change a level of logs that will be sent to stdout.
   */
  static void SetStdLogLevel(LogLevel level);

 private:
  /**
   * Internal wrappers for printing to PostMessage with specified prefix.
   */
  static void InternalPrint(LogLevel, const char* std_prefix,
                            const char* message);
  static void InternalPrint(LogLevel, const char* std_prefix,
                            const char* message_format, va_list arguments_list);
  static void InternalPrint(int line, const char* func, const char* file,
                            LogLevel, const char* message_format,
                            va_list arguments_list);
  static void InternalPrint(LogLevel level, const std::string& message) {
    InternalPrint(level, nullptr, message.c_str());
  }

  static bool IsLoggingEnabled() {
    return std_log_level_ != LogLevel::kNone ||
           js_log_level_ != LogLevel::kNone;
  }

  static pp::Instance* instance_;
  static LogLevel js_log_level_;
  static LogLevel std_log_level_;
};

#endif  // COMMON_SRC_LOGGER_H_
