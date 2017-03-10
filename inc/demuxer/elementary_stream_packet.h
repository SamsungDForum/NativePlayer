/*!
 * elementary_stream_packet.h (https://github.com/SamsungDForum/NativePlayer)
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
 */

#ifndef SRC_PLAYER_ES_DASH_PLAYER_DEMUXER_ELEMENTARY_STREAM_PACKET_H_
#define SRC_PLAYER_ES_DASH_PLAYER_DEMUXER_ELEMENTARY_STREAM_PACKET_H_

#include <vector>

#include "nacl_player/media_common.h"

/// @file
/// @brief This file defines the <code>ElementaryStreamPacket</code>.

/// @class ElementaryStreamPacket
/// @brief This class is a wrapper for both Samsung::NaClPlayer::ESPacket and
/// encryption information.
///
/// <code>ElementaryStreamPacket</code> is used to extract data from
/// <code>StreamDemuxer</code> and can be used to pass it further.
/// @see Samsung::NaClPlayer::ESPacket
/// @see Samsung::NaClPlayer::EncryptedSubsampleDescription
/// @see Samsung::NaClPlayer::ESPacketEncryptionInfo
class ElementaryStreamPacket {
 public:
  /// Constructs <code>ElementaryStreamPacket</code> and
  /// initialize Samsung::NaClPlayer::ESPacket with given data.
  ///
  /// @param[in] data A byte array which helds data of elementary stream
  ///   packet. It is copied to the internal byte array.
  /// @param[in] size A size of data array in bytes.
  /// @see Samsung::NaClPlayer::ESPacket
  ElementaryStreamPacket(uint8_t* data, uint32_t size);

  ElementaryStreamPacket(const ElementaryStreamPacket&) = delete;

  /// Move-constructs a <code>ElementaryStreamPacket</code> object,
  /// making it point at the same object that <code>other</code> was pointing
  /// to.
  ElementaryStreamPacket(ElementaryStreamPacket&& other) = default;

  /// Destroys <code>ElementaryStreamPacket</code> object.
  ~ElementaryStreamPacket() = default;

  ElementaryStreamPacket& operator=(const ElementaryStreamPacket&) = delete;

  /// Move-assigns <code>other</code> to this
  /// <code>ElementaryStreamPacket</code> object.
  ElementaryStreamPacket& operator=(ElementaryStreamPacket&& other) = default;

  /// Returns Elementary Stream Packet.
  const Samsung::NaClPlayer::ESPacket& GetESPacket() const;

  /// Returns Encryption information.
  const Samsung::NaClPlayer::ESPacketEncryptionInfo& GetEncryptionInfo() const;

  /// Returns true if packet is encrypted, otherwise false.
  bool IsEncrypted() const;

  /// Returns true if packet is a key frame, otherwise false.
  /// @see Samsung::NaClPlayer::ESPacket::is_key_frame
  bool IsKeyFrame() const { return es_packet_.is_key_frame; }

  /// Returns size of packet's data.
  uint32_t GetDataSize() const { return data_.size(); }

  /// Returns the presentation timestamp.
  /// @see Samsung::NaClPlayer::ESPacket::pts
  Samsung::NaClPlayer::TimeTicks GetPts() const { return es_packet_.pts; }

  /// Returns the decode timestamp.
  /// @see Samsung::NaClPlayer::ESPacket::dts
  Samsung::NaClPlayer::TimeTicks GetDts() const { return es_packet_.dts; }

  /// Returns the packet's duration.
  /// @see Samsung::NaClPlayer::ESPacket::duration
  Samsung::NaClPlayer::TimeTicks GetDuration() const {
    return es_packet_.duration;
  }

  /// Allows to set if packet is a key frame or not.
  /// @see Samsung::NaClPlayer::ESPacket::is_key_frame
  void SetKeyFrame(bool key_frame) { es_packet_.is_key_frame = key_frame; }

  /// Allows to set a presentation timestamp.
  /// @see Samsung::NaClPlayer::ESPacket::pts
  void SetPts(Samsung::NaClPlayer::TimeTicks pts) { es_packet_.pts = pts; }

  /// Allows to set a decode timestamp.
  /// @see Samsung::NaClPlayer::ESPacket::dts
  void SetDts(Samsung::NaClPlayer::TimeTicks dts) { es_packet_.dts = dts; }

  /// Allows to set a packet's duration.
  /// @see Samsung::NaClPlayer::ESPacket::duration
  void SetDuration(Samsung::NaClPlayer::TimeTicks duration) {
    es_packet_.duration = duration;
  }

  /// Sets a key id for encrypted data needed to decrypt it.
  ///
  /// @param[in] key_id An byte array which helds data of key id. It is copied
  ///   to the internal byte array.
  /// @param[in] key_id_size A size of key_id array in bytes.
  void SetKeyId(uint8_t* key_id, uint32_t key_id_size);

  /// Sets an initialization vector for encrypted data needed to decrypt it.
  ///
  /// @param[in] iv An byte array which helds data of initialization vector. It
  ///   is copied to the internal byte array.
  /// @param[in] iv_size A size of iv array in bytes.
  void SetIv(uint8_t* iv, uint32_t iv_size);

  /// Clears the subsample information about encrypted bytes in packet.
  void ClearSubsamples();

  /// Adds a subsample information about encrypted bytes in packet.
  ///
  /// @param[in] clear_bytes A count of unencrypted bytes in elementary
  /// stream packet.
  /// @param[in] cipher_bytes A count of encrypted bytes in elementary
  /// stream packet.
  void AddSubsample(uint32_t clear_bytes, uint32_t cipher_bytes);

  int demux_id;

 private:
  // assumption: Address returned by vector::data() method is invariant under
  //             move operations

  // invariants:

  // es_packet.data == data_.data() && es_packet.size == data.size()
  void FixDataInvariant();

  // encryption_info.key_id == key_id_.data()
  // encryption_info.key_id_size == key_id_.size()
  void FixKeyIdInvariant();

  // encryption_info.iv == key_id_.data()
  // encryption_info.iv_size == key_id_.size()
  void FixIvInvariant();

  // encryption_info.subsamples == subsamples_.data()
  // encryption_info.num_subsamples == subsamples_.size()
  void FixSubsamplesInvariant();

  std::vector<uint8_t> data_;
  Samsung::NaClPlayer::ESPacket es_packet_;

  std::vector<uint8_t> key_id_;
  std::vector<uint8_t> iv_;
  std::vector<Samsung::NaClPlayer::EncryptedSubsampleDescription> subsamples_;
  Samsung::NaClPlayer::ESPacketEncryptionInfo encryption_info_;
};

#endif  // SRC_PLAYER_ES_DASH_PLAYER_DEMUXER_ELEMENTARY_STREAM_PACKET_H_
