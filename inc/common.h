/*!
 * common.h (https://github.com/SamsungDForum/NativePlayer)
 * Copyright 2016, Samsung Electronics Co., Ltd
 * Licensed under the MIT license
 *
 * @author Tomasz Borkowski
 * @author Piotr Bałut
 */

#ifndef NATIVE_PLAYER_SRC_COMMON_H_
#define NATIVE_PLAYER_SRC_COMMON_H_

#include <functional>
#include <limits>
#include <memory>
#include <sstream>
#include <string>
#include <utility>

#include "nacl_player/common.h"

#include "dash/media_stream.h"

#include "logger.h"

#define LOG_STATS __LINE__, __func__, __FILE__
#define LOG_INFO(msg, ...) Logger::Info(LOG_STATS, msg, ##__VA_ARGS__)
#define LOG_ERROR(msg, ...) Logger::Error(LOG_STATS, msg, ##__VA_ARGS__)
#define LOG_DEBUG(msg, ...) Logger::Debug(LOG_STATS, msg, ##__VA_ARGS__)

constexpr double kEps = 0.0001;
constexpr Samsung::NaClPlayer::TimeTicks kSegmentMargin = 0.1;

template <typename T, class... Args>
std::unique_ptr<T> MakeUnique(Args&&... args) {
  return std::unique_ptr<T>(new T(std::forward<Args>(args)...));
}

template <typename T>
std::unique_ptr<T> AdoptUnique(T* ptr) {
  return std::unique_ptr<T>(ptr);
}

enum class StreamType : int32_t {
  Video = static_cast<int32_t>(MediaStreamType::Video),
  Audio = static_cast<int32_t>(MediaStreamType::Audio),
  MaxStreamTypes = static_cast<int32_t>(MediaStreamType::MaxTypes)
};

const Samsung::NaClPlayer::TimeTicks kEndOfStream =
    std::numeric_limits<Samsung::NaClPlayer::TimeTicks>::infinity();

inline std::string ToHexString(uint32_t size, const uint8_t* data) {
  std::ostringstream oss;
  oss.setf(std::ios::hex, std::ios::basefield);
  for (size_t i = 0; i < size; ++i) {
    oss.width(2);
    oss.fill('0');
    oss << static_cast<unsigned>(data[i]);
    oss << " ";
  }

  return oss.str();
}

// From:
// https://isocpp.org/wiki/faq/pointers-to-members#macro-for-ptr-to-memfn
#define CALL_MEMBER_FN(object, ptr_to_member)  ((object).*(ptr_to_member))

template <typename MethodSignature>
struct WeakBindHelper {
  static constexpr bool has_valid_signature = false;
};

template <typename Ret, typename ClassT, typename ... Args>
struct WeakBindHelper<Ret(ClassT::*)(Args...)> {
  static constexpr bool has_valid_signature = true;

  template<typename MethodT>
  static void WeakCall(
      MethodT method, std::weak_ptr<ClassT> weak_class_ptr, Args&& ... args) {
    if (auto class_ptr = weak_class_ptr.lock()) {
      CALL_MEMBER_FN(*class_ptr, method)(std::forward<Args>(args)...);
    } else {
      LOG_ERROR("A call to a dead object, ignoring.");
    }
  }
};

// An std::bind equivalent that binds to the method of the object referenced by
// a std::weak_ptr. If object is deleted, a call made to the resulting functor
// will be ignored.
template<typename MethodT, typename ClassT, typename ... Args>
inline auto WeakBind(
    MethodT method, const std::shared_ptr<ClassT>& class_ptr, Args&& ... args)
-> decltype(std::bind(&WeakBindHelper<MethodT>::template WeakCall<MethodT>,
    method, std::weak_ptr<ClassT>(class_ptr), std::forward<Args>(args)...)) {
  static_assert(WeakBindHelper<MethodT>::has_valid_signature,
      "\"method\" is not a valid method pointer!");
  return std::bind(
      &WeakBindHelper<MethodT>::template WeakCall<MethodT>,
      method, std::weak_ptr<ClassT>(class_ptr),
      std::forward<Args>(args)...);
}

#endif  // NATIVE_PLAYER_SRC_COMMON_H_
