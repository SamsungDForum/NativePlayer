/*!
 * common.cc (https://github.com/SamsungDForum/NativePlayer)
 * Copyright 2016, Samsung Electronics Co., Ltd
 * Licensed under the MIT license
 *
 * @author Adam Bujalski
 */

#include <cassert>

#include "ppapi/c/pp_errors.h"
#include "ppapi/cpp/completion_callback.h"
#include "ppapi/cpp/instance_handle.h"
#include "ppapi/cpp/message_loop.h"
#include "ppapi/cpp/module.h"
#include "ppapi/cpp/url_loader.h"
#include "ppapi/cpp/url_response_info.h"

#include "common.h"
#include "logger.h"

namespace {

constexpr uint32_t kMinBufferSize = 64 * 1024;
constexpr uint32_t kExtendBufferSize = 256 * 1024;

inline pp::InstanceHandle CurrentInstanceHandle() {
  pp::Module* module = pp::Module::Get();
  if (!module) return pp::InstanceHandle(static_cast<PP_Instance>(0));

  if (module->current_instances().empty())
    return pp::InstanceHandle(static_cast<PP_Instance>(0));

  return pp::InstanceHandle(module->current_instances().begin()->first);
}

template<typename T>
int32_t ProcessURLRequest(const pp::URLRequestInfo& request, T* out) {
  if (out == nullptr)
    return PP_ERROR_BADARGUMENT;

  out->clear();
  if (pp::MessageLoop::GetCurrent().is_null())
    return PP_ERROR_NO_MESSAGE_LOOP;

  if (pp::MessageLoop::GetCurrent() == pp::MessageLoop::GetForMainThread())
    return PP_ERROR_BLOCKS_MAIN_THREAD;

  if (request.is_null()) {
    LOG_ERROR("request is null!");
    return PP_ERROR_BADARGUMENT;
  }

  pp::URLLoader loader(CurrentInstanceHandle());
  int32_t ret = loader.Open(request, pp::CompletionCallback());
  if (ret != PP_OK) {
    LOG_ERROR("Failed to open URLLoader with given request, code: %d", ret);
    return ret;
  }

  pp::URLResponseInfo response_info(loader.GetResponseInfo());
  if (response_info.is_null()) {
    LOG_ERROR("URLLoader::GetResponseInfo returned null");
    return PP_ERROR_FAILED;
  }

  int32_t status_code = response_info.GetStatusCode();
  if (status_code >= 400) {
    LOG_ERROR("Unexpected HTTP status code: %d", status_code);
    return PP_ERROR_FAILED;
  }

  size_t bytes_received = 0;
  while (true) {
    if (out->size() < bytes_received + kMinBufferSize)
      out->resize(bytes_received + kExtendBufferSize);

    if (out->size() <= bytes_received) {
      out->clear();
      LOG_ERROR("Failed to resize buffer");
      return PP_ERROR_NOMEMORY;
    }

    size_t bytes_to_read = out->size() - bytes_received;
    ret = loader.ReadResponseBody(&(*out)[bytes_received],
                                  bytes_to_read,
                                  pp::CompletionCallback());
    if (ret < 0) {
      out->clear();
      LOG_ERROR("Failed to ReadResponseBody, result: %d", ret);
      return PP_ERROR_FAILED;
    }

    if (ret == PP_OK) break;

    bytes_received += ret;
  }

  out->resize(bytes_received);
  return PP_OK;
}

}  // namespace

std::string ToHexString(uint32_t size, const uint8_t* data) {
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

// Simple implementation based on https://en.wikipedia.org/wiki/Base64
std::vector<uint8_t> Base64Decode(const std::string& text) {
  if (text.length() % 4) {
    LOG_ERROR("Invalid base64 input - not divisible by 4");
    return {};
  }

  static const std::string codes =
    "ABCDEFGHIJKLMNOPQRSTUVWXYZabcdefghijklmnopqrstuvwxyz0123456789+/=";

  size_t pos = text.find('=');
  uint32_t decoded_size = (text.length() * 3) / 4;
  if (pos != std::string::npos) decoded_size -= text.length() - pos;

  std::vector<uint8_t> ret(decoded_size);
  uint32_t j = 0;
  uint32_t b[4];
  for (uint32_t i = 0; i < text.length(); i += 4) {
    for (uint32_t k = 0; k < 4; ++k) {
      pos = codes.find(text[i + k]);
      if (pos == std::string::npos) {
        LOG_ERROR("Bad character found in input");
        return {};
      }

      b[k] = pos;
    }

    ret[j++] = ((b[0] << 2) | (b[1] >> 4));
    if (b[2] >= 64) continue;

    ret[j++] = ((b[1] << 4) | (b[2] >> 2));
    if (b[3] >= 64) continue;

    ret[j++] = ((b[2] << 6)) | b[3];
  }

  return ret;
}

pp::URLRequestInfo GetRequestForURL(const std::string& url) {
  pp::URLRequestInfo request(CurrentInstanceHandle());
  request.SetURL(url);
  return request;
}

int32_t ProcessURLRequestOnSideThread(const pp::URLRequestInfo& request,
                                      std::string* out) {
  return ProcessURLRequest(request, out);
}

int32_t ProcessURLRequestOnSideThread(const pp::URLRequestInfo& request,
                                      std::vector<uint8_t>* out) {
  return ProcessURLRequest(request, out);
}

