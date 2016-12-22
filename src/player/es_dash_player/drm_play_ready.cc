/*!
 * drm_play_ready.cc (https://github.com/SamsungDForum/NativePlayer)
 * Copyright 2016, Samsung Electronics Co., Ltd
 * Licensed under the MIT license
 *
 * @author Tomasz Borkowski
 */

#include <algorithm>
#include <sstream>

#include "ppapi/c/pp_errors.h"
#include "ppapi/cpp/completion_callback.h"
#include "ppapi/cpp/url_loader.h"
#include "ppapi/cpp/url_response_info.h"
#include "ppapi/cpp/url_request_info.h"

#include "drm_play_ready.h"

#include "common.h"
#include "libdash/libdash.h"

using std::shared_ptr;
using std::string;
using std::vector;

using pp::CompletionCallback;
using pp::URLLoader;
using pp::URLResponseInfo;
using pp::URLRequestInfo;
using Samsung::NaClPlayer::DRMOperation_InstallLicense;
using Samsung::NaClPlayer::DRMType;
using Samsung::NaClPlayer::DRMType_Playready;
using Samsung::NaClPlayer::ErrorCodes;

using dash::mpd::IDescriptor;

const char* kAttributeNameType = "type";
const char* kPlayReadyType = "playready";
const char* kXMLTag = "<?xml";
const char* kSoapTagEnd = "</soap:Envelope>";
const char* kPlayreadSchemeIdUri =
    "urn:uuid:9a04f079-9840-4286-ab92-e65be0885f95";
const char* kSchemeIdUriAttribute = "schemeIdUri";
const char* kCencPsshAttribute = "cenc:pssh";

DrmPlayReadyListener::DrmPlayReadyListener(
    const pp::InstanceHandle& instance,
    std::shared_ptr<Samsung::NaClPlayer::MediaPlayer> player)
    : instance_(instance),
      cc_factory_(this),
      player_(player),
      pending_licence_requests_(0) {
  side_thread_loop_ = pp::MessageLoop::GetCurrent();
}

void DrmPlayReadyListener::OnInitdataLoaded(DRMType drm_type,
                                            uint32_t init_data_size,
                                            const void* init_data) {
  LOG_INFO("drm_type: %d, init_data_size: %d", drm_type, init_data_size);
  LOG_DEBUG("init_data str: [[%s]]",
            ToHexString(init_data_size,
                        static_cast<const uint8_t*>(init_data)).c_str());
}

void DrmPlayReadyListener::OnLicenseRequest(uint32_t request_size,
                                            const void* request) {
  LOG_INFO("Making license request to: %s",
           cp_descriptor_->system_url_.c_str());
  LOG_DEBUG("request_size: %d, str: [%s]", request_size, request);
  std::string soap_request(static_cast<const char*>(request),
                           static_cast<const char*>(request) + request_size);

  // Clear garbage at the end...
  size_t soap_end = soap_request.find(kSoapTagEnd);
  if (soap_end != std::string::npos) {
    soap_request.resize(soap_end + strlen(kSoapTagEnd));
  }

  ++pending_licence_requests_;

  URLRequestInfo lic_request = GetRequestForURL(cp_descriptor_->system_url_);
  lic_request.SetMethod("POST");
  lic_request.AppendDataToBody(soap_request.data(), soap_request.size());
  if (!cp_descriptor_->key_request_properties_.empty()) {
    std::ostringstream oss;
    for (const auto& e : cp_descriptor_->key_request_properties_)
      oss << e.first << ": " << e.second << "\n";

    lic_request.SetHeaders(oss.str());
  }

  side_thread_loop_.PostWork(cc_factory_.NewCallback(
      &DrmPlayReadyListener::ProcessLicenseRequestOnSideThread,
      cp_descriptor_->system_url_, lic_request));

  LOG_DEBUG("Redirected license request to a side thread");
}

void DrmPlayReadyListener::ProcessLicenseRequestOnSideThread(
    int32_t, const std::string& url, URLRequestInfo lic_request) {
  LOG_DEBUG("Start");
  std::string response;
  int32_t ret = ProcessURLRequestOnSideThread(lic_request, &response);
  if (ret != PP_OK) {
    LOG_ERROR("Failed to download license from: %s result: %d",
              url.c_str(), ret);
    return;
  }

  LOG_INFO("Successfully retrieved license request!");
  // Some servers (e.g. YouTube)
  // put data into HTTP body before XML;
  // this tries to skip this data
  response.erase(0, response.find(kXMLTag));
  LOG_DEBUG("response after removing headers:\n%s", response.c_str());

  ret = player_->SetDRMSpecificData(DRMType_Playready,
                                    DRMOperation_InstallLicense,
                                    response.size(), response.data());
  if (ret != ErrorCodes::Success) {
    LOG_ERROR("Failed to install license!, code: %d", ret);
    return;
  }

  if (pending_licence_requests_)
    --pending_licence_requests_;

  LOG_INFO("Successfully installed license.");
}

bool DrmPlayReadyListener::IsInitialized() const {
  return pending_licence_requests_.load() == 0;
}

void DrmPlayReadyListener::Reset() {
  pending_licence_requests_ = 0;
}


shared_ptr<ContentProtectionDescriptor>
DrmPlayReadyContentProtectionVisitor::Visit(const vector<IDescriptor*>& cp) {
  if (cp.empty()) return {};

  auto desc = std::make_shared<DrmPlayReadyContentProtectionDescriptor>();

  // search IDescriptor elements
  for (auto mpd_desc : cp) {
    desc->scheme_id_uri_ = mpd_desc->GetSchemeIdUri();
    std::transform(desc->scheme_id_uri_.begin(),
        desc->scheme_id_uri_.end(),
        desc->scheme_id_uri_.begin(),
        [](unsigned char c) { return std::tolower(c); });

    if (desc->scheme_id_uri_ == kPlayreadSchemeIdUri) {
      for (auto n : mpd_desc->GetAdditionalSubNodes()) {
        if (n->GetName() == kCencPsshAttribute) {
          desc->init_data_type_ = n->GetName();
          desc->init_data_ = Base64Decode(n->GetText());
        }
      }

      return desc;
    }

    // search INode elements
    for (const auto& sub_node : mpd_desc->GetAdditionalSubNodes()) {
      if (sub_node->GetAttributeValue(kAttributeNameType) == kPlayReadyType) {
        desc->system_url_ = sub_node->GetText();
        LOG_DEBUG("found playready content protection! url: %s",
                  desc->system_url_.c_str());
        return desc;
      }
    }
  }

  LOG_DEBUG("No playready content protection");
  return {};  // we support only PlayReady here
}
