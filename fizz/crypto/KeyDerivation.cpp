/*
 *  Copyright (c) 2018-present, Facebook, Inc.
 *  All rights reserved.
 *
 *  This source code is licensed under the BSD-style license found in the
 *  LICENSE file in the root directory of this source tree.
 */

#include <fizz/crypto/KeyDerivation.h>

namespace fizz {

Buf KeyDerivationImpl::expandLabel(
    folly::ByteRange secret,
    folly::StringPiece label,
    Buf hashValue,
    uint16_t length) {
  HkdfLabel hkdfLabel = {
      length, std::string(label.begin(), label.end()), std::move(hashValue)};
  return hkdf_.expand(
      secret, *encodeHkdfLabel(std::move(hkdfLabel), kHkdfLabelPrefix), length);
}

Buf KeyDerivationImpl::hkdfExpand(
    folly::ByteRange secret,
    Buf info,
    uint16_t length) {
  return hkdf_.expand(secret, *info, length);
}

std::vector<uint8_t> KeyDerivationImpl::deriveSecret(
    folly::ByteRange secret,
    folly::StringPiece label,
    folly::ByteRange messageHash,
    uint16_t length) {
  auto hlen = hkdf_.hasher()->hashLength();
  CHECK_EQ(secret.size(), hlen);
  CHECK_EQ(messageHash.size(), hlen);
  CHECK_GT(length, 0);
  // Copying the buffer to avoid violating constness of the data.
  auto hashBuf = folly::IOBuf::copyBuffer(messageHash);
  auto out = expandLabel(secret, label, std::move(hashBuf), length);
  std::vector<uint8_t> prk(length);
  size_t offset = 0;
  for (auto buf : *out) {
    size_t remaining = length - offset;
    size_t copyLength = std::min(buf.size(), remaining);
    memcpy(prk.data() + offset, buf.data(), copyLength);
    offset += copyLength;
  }
  return prk;
}
} // namespace fizz
