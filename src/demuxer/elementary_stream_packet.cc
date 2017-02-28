/*!
 * elementary_stream_packet.cc (https://github.com/SamsungDForum/NativePlayer)
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

#include "demuxer/elementary_stream_packet.h"

using Samsung::NaClPlayer::EncryptedSubsampleDescription;
using Samsung::NaClPlayer::ESPacket;
using Samsung::NaClPlayer::ESPacketEncryptionInfo;
using Samsung::NaClPlayer::TimeTicks;

ElementaryStreamPacket::ElementaryStreamPacket(uint8_t* data, uint32_t size)
    : data_(data, data + size) {
  FixDataInvariant();
  FixKeyIdInvariant();
  FixIvInvariant();
  FixSubsamplesInvariant();
}

const ESPacket& ElementaryStreamPacket::GetESPacket() const {
  return es_packet_;
}

const ESPacketEncryptionInfo& ElementaryStreamPacket::GetEncryptionInfo()
    const {
  return encryption_info_;
}

bool ElementaryStreamPacket::IsEncrypted() const {
  // There might be 0 subsamples in encrypted packet.
  return !key_id_.empty() || !iv_.empty();
}

void ElementaryStreamPacket::SetKeyId(uint8_t* key_id, uint32_t key_id_size) {
  if (key_id && key_id_size)
    key_id_.assign(key_id, key_id + key_id_size);
  else
    key_id_.clear();

  FixKeyIdInvariant();
}

void ElementaryStreamPacket::SetIv(uint8_t* iv, uint32_t iv_size) {
  if (iv && iv_size)
    iv_.assign(iv, iv + iv_size);
  else
    iv_.clear();

  FixIvInvariant();
}

void ElementaryStreamPacket::ClearSubsamples() {
  subsamples_.clear();
  FixSubsamplesInvariant();
}

void ElementaryStreamPacket::AddSubsample(uint32_t clear_bytes,
                                          uint32_t cipher_bytes) {
  EncryptedSubsampleDescription subsample = {clear_bytes, cipher_bytes};
  subsamples_.push_back(subsample);
  FixSubsamplesInvariant();
}

// es_packet.data == data_.data() && es_packet.size == data.size()
void ElementaryStreamPacket::FixDataInvariant() {
  es_packet_.buffer = data_.data();
  es_packet_.size = data_.size();
}

void ElementaryStreamPacket::FixKeyIdInvariant() {
  encryption_info_.key_id = key_id_.data();
  encryption_info_.key_id_size = key_id_.size();
}

void ElementaryStreamPacket::FixIvInvariant() {
  encryption_info_.iv = iv_.data();
  encryption_info_.iv_size = iv_.size();
}

void ElementaryStreamPacket::FixSubsamplesInvariant() {
  encryption_info_.subsamples = subsamples_.data();
  encryption_info_.num_subsamples = subsamples_.size();
}
