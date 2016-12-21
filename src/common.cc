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
    LOG_ERROR("Failed to open URLLoader with license request, code: %d", ret);
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

  char buf[32 * 1024];
  while (true) {
    ret = loader.ReadResponseBody(buf, sizeof(buf), pp::CompletionCallback());
    if (ret < 0) {
      LOG_ERROR("Failed to ReadResponseBody, result: %d", ret);
      return PP_ERROR_FAILED;
    }

    if (ret == PP_OK) break;

    out->insert(out->end(), buf, buf + ret);
  }

  loader.Close();
  return PP_OK;
}

}  // namespace

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

