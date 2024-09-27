/*
 *  Copyright (c) 2019-present, Facebook, Inc.
 *  All rights reserved.
 *
 *  This source code is licensed under the BSD-style license found in the
 *  LICENSE file in the root directory of this source tree.
 */

#include <fizz/crypto/hpke/Hkdf.h>
#include <fizz/record/Types.h>

using namespace fizz::detail;

namespace fizz {
namespace hpke {

constexpr folly::StringPiece prefixV1("HPKE-v1");

Hkdf Hkdf::v1(const HasherFactoryWithMetadata* hash) {
  return Hkdf(prefixV1, hash);
}

Hkdf Hkdf::withPrefix(
    folly::ByteRange prefix,
    const HasherFactoryWithMetadata* hash) {
  return Hkdf(prefix, hash);
}

std::vector<uint8_t> Hkdf::labeledExtract(
    Buf salt,
    folly::ByteRange label,
    Buf ikm,
    std::unique_ptr<folly::IOBuf> suiteId) {
  Buf labeledIkm = folly::IOBuf::create(
      prefix_.size() + suiteId->computeChainDataLength() + label.size() +
      ikm->computeChainDataLength());
  folly::io::Appender appender(labeledIkm.get(), 0);

  appender.push(prefix_);
  writeBufWithoutLength(suiteId, appender);
  writeBufWithoutLength(folly::IOBuf::wrapBuffer(label), appender);
  writeBufWithoutLength(ikm, appender);

  return hkdf_.extract(salt->coalesce(), labeledIkm->coalesce());
}

std::vector<uint8_t> Hkdf::extract(Buf salt, Buf ikm) {
  return hkdf_.extract(salt->coalesce(), ikm->coalesce());
}

std::unique_ptr<folly::IOBuf> Hkdf::labeledExpand(
    folly::ByteRange prk,
    folly::ByteRange label,
    Buf info,
    size_t L,
    std::unique_ptr<folly::IOBuf> suiteId) {
  Buf labeledInfo = folly::IOBuf::create(
      2 + prefix_.size() + suiteId->computeChainDataLength() + label.size() +
      info->computeChainDataLength());
  folly::io::Appender appender(labeledInfo.get(), 0);

  if (L > (1 << 16) - 1) {
    throw std::runtime_error("This is greater than the maximum length allowed");
  }
  appender.writeBE<uint16_t>(L);
  appender.push(prefix_);
  writeBufWithoutLength(suiteId, appender);
  writeBufWithoutLength(folly::IOBuf::wrapBuffer(label), appender);
  writeBufWithoutLength(info, appender);

  return hkdf_.expand(prk, *labeledInfo, L);
}

std::unique_ptr<folly::IOBuf> Hkdf::expand(
    folly::ByteRange prk,
    std::unique_ptr<folly::IOBuf> label,
    size_t L) {
  return hkdf_.expand(prk, *label, L);
}

size_t Hkdf::hashLength() {
  return hkdf_.hashLength();
}

} // namespace hpke
} // namespace fizz
