/*!
 * message_sender.cc (https://github.com/SamsungDForum/NativePlayer)
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
 * @author Tomasz Borkowski
 * @author Michal Murgrabia
 */

#include "communicator/message_sender.h"

#include <string>

#include "ppapi/cpp/var_dictionary.h"

#include "communicator/messages.h"

using pp::Var;
using pp::VarDictionary;
using Samsung::NaClPlayer::TimeTicks;
using Samsung::NaClPlayer::TextTrackInfo;

namespace Communication {

void MessageSender::SetMediaDuration(TimeTicks duration) {
  VarDictionary message;
  message.Set(kKeyMessageFromPlayer, MessageFromPlayer::kSetDuration);
  message.Set(kKeyTime, duration);
  PostMessage(message);
}

void MessageSender::CurrentTimeUpdate(TimeTicks time) {
  LOG_DEBUG("Current clip time: %f", time);
  VarDictionary message;
  message.Set(kKeyMessageFromPlayer, MessageFromPlayer::kTimeUpdate);
  message.Set(kKeyTime, time);
  PostMessage(message);
}

void MessageSender::BufferingCompleted() {
  VarDictionary message;
  message.Set(kKeyMessageFromPlayer, MessageFromPlayer::kBufferingCompleted);
  PostMessage(message);
}

void MessageSender::SetRepresentations(const std::vector<AudioStream>& reps) {
  for (auto rep : reps) {
    VarDictionary message;
    message.Set(kKeyMessageFromPlayer,
                static_cast<int>(MessageFromPlayer::kAudioRepresentation));
    message.Set(kKeyId, static_cast<int>(rep.description.id));
    message.Set(kKeyBitrate,
                static_cast<int>(rep.description.bitrate));
    message.Set(kKeyLanguage, rep.language);
    PostMessage(message);
  }
}

void MessageSender::SetRepresentations(const std::vector<VideoStream>& reps) {
  for (const auto& rep : reps) {
    VarDictionary message;
    message.Set(kKeyMessageFromPlayer,
                static_cast<int>(MessageFromPlayer::kVideoRepresentation));
    message.Set(kKeyId, static_cast<int>(rep.description.id));
    message.Set(kKeyBitrate,
                static_cast<int>(rep.description.bitrate));
    message.Set(kKeyHeight, static_cast<int>(rep.height));
    message.Set(kKeyWidth, static_cast<int>(rep.width));
    PostMessage(message);
  }
}

void MessageSender::ChangeRepresentation(StreamType type, int32_t id) {
  VarDictionary message;
  message.Set(kKeyMessageFromPlayer,
              static_cast<int>(MessageFromPlayer::kRepresentationChanged));
  message.Set(kKeyType, static_cast<int>(type));
  message.Set(kKeyId, id);
  PostMessage(message);
}

void MessageSender::ShowSubtitles(TimeTicks duration, const Var& text) {
  VarDictionary message;
  message.Set(kKeyMessageFromPlayer,
              static_cast<int>(MessageFromPlayer::kSubtitles));
  message.Set(kKeyDuration, duration);
  message.Set(kKeySubtitle, text);
  LOG_DEBUG("Sending to JS text: %s", text.AsString().c_str());
  PostMessage(message);
}

void MessageSender::SetTextTracks(const std::vector<TextTrackInfo>& reps) {
  for (const auto& rep : reps) {
    VarDictionary message;
    message.Set(kKeyMessageFromPlayer,
      static_cast<int>(MessageFromPlayer::kSubtitlesRepresentation));
    message.Set(kKeyId, static_cast<int>(rep.index));
    message.Set(kKeyLanguage, rep.language);
    PostMessage(message);
  }
}

void MessageSender::StreamEnded() {
  VarDictionary message;
  message.Set(kKeyMessageFromPlayer, MessageFromPlayer::kStreamEnded);
  PostMessage(message);
}

void MessageSender::PostMessage(const Var& message) {
  instance_->PostMessage(message);
}

}  // namespace Communication
