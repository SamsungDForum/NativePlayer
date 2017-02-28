/*!
 * ffmpeg_demuxer.h (https://github.com/SamsungDForum/NativePlayer)
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
 * @author Jacob Tarasiewicz
 * @author Tomasz Borkowski
 */

#ifndef SRC_PLAYER_ES_DASH_PLAYER_DEMUXER_FFMPEG_DEMUXER_H_
#define SRC_PLAYER_ES_DASH_PLAYER_DEMUXER_FFMPEG_DEMUXER_H_

#include <condition_variable>
#include <functional>
#include <list>
#include <memory>
#include <mutex>
#include <string>
#include <thread>
#include <tuple>
#include <vector>

#include "ppapi/cpp/message_loop.h"
#include "ppapi/utility/completion_callback_factory.h"
#include "nacl_player/media_common.h"

extern "C" {
#include "libavformat/avformat.h"
}

#include "demuxer/stream_demuxer.h"

class FFMpegDemuxer : public StreamDemuxer {
 public:
  typedef std::function<void(StreamDemuxer::Message,
      std::unique_ptr<ElementaryStreamPacket>)> InitCallback;
  typedef std::function<void(const std::string&,
      const std::vector<uint8_t>& init_data)> DrmInitCallback;

  explicit FFMpegDemuxer(const pp::InstanceHandle& instance,
                         uint32_t probe_size, Type type, InitMode init_mode);
  ~FFMpegDemuxer();

  bool Init(const InitCallback& callback,
            pp::MessageLoop callback_dispatcher) override;
  void Flush() override;
  void Parse(const std::vector<uint8_t>& data) override;
  bool SetAudioConfigListener(
      const std::function<void(const AudioConfig&)>& callback) override;
  bool SetVideoConfigListener(
      const std::function<void(const VideoConfig&)>& callback) override;
  bool SetDRMInitDataListener(const DrmInitCallback& callback) override;
  void SetTimestamp(Samsung::NaClPlayer::TimeTicks) override;
  void Close() override;
  int Read(uint8_t* data, int size);

 private:
  typedef std::tuple<
      StreamDemuxer::Message,
      std::unique_ptr<ElementaryStreamPacket>> EsPktCallbackData;
  static constexpr uint32_t kEsPktCallbackDataMessage = 0;
  static constexpr uint32_t kEsPktCallbackDataPacket = 1;

  // main parser thread function
  void ParsingThreadFn();

  void CallbackInDispatcherThread(int32_t, StreamDemuxer::Message msg);
  void DispatchCallback(StreamDemuxer::Message);
  void EsPktCallbackInDispatcherThread(int32_t,
      const std::shared_ptr<EsPktCallbackData>& data);
  void DrmInitCallbackInDispatcherThread(int32_t, const std::string& type,
      const std::vector<uint8_t>& init_data);
  bool InitStreamInfo();
  static void InitFFmpeg();

  std::unique_ptr<ElementaryStreamPacket> MakeESPacketFromAVPacket(
      AVPacket* pkt);

  int PrepareAACHeader(const uint8_t* data, size_t length);
  void UpdateVideoConfig();
  void UpdateAudioConfig();
  void UpdateContentProtectionConfig();
  void CallbackConfigInDispatcherThread(int32_t, Type type);
  std::function<void(const VideoConfig&)> video_config_callback_;
  std::function<void(const AudioConfig&)> audio_config_callback_;
  std::function<void(const std::string& type,
                     const std::vector<uint8_t>& init_data)>
      drm_init_data_callback_;
  std::function<void(StreamDemuxer::Message,
                     std::unique_ptr<ElementaryStreamPacket>)>
      es_pkt_callback_;

  Type stream_type_;
  int audio_stream_idx_;
  int video_stream_idx_;
  std::unique_ptr<std::thread> parser_thread_;
  pp::CompletionCallbackFactory<FFMpegDemuxer> callback_factory_;

  VideoConfig video_config_;
  AudioConfig audio_config_;
  AVFormatContext* format_context_;
  AVIOContext* io_context_;

  std::mutex buffer_mutex_;
  std::condition_variable buffer_condition_;
  pp::MessageLoop callback_dispatcher_;
  std::vector<uint8_t> buffer_;
  // in case of performance issues, buffer_ may be changed to a list of
  // buffers to reduce amount of data copying
  bool context_opened_;
  bool streams_initialized_;
  bool end_of_file_;
  bool exited_;
  uint32_t probe_size_;
  Samsung::NaClPlayer::TimeTicks timestamp_;
  bool has_packets_;
  InitMode init_mode_;

  int demux_id_;
};

#endif  // SRC_PLAYER_ES_DASH_PLAYER_DEMUXER_FFMPEG_DEMUXER_H_
