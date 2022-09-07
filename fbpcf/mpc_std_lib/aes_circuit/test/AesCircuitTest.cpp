/*
 * Copyright (c) Meta Platforms, Inc. and affiliates.
 *
 * This source code is licensed under the MIT license found in the
 * LICENSE file in the root directory of this source tree.
 */

#include <gmock/gmock.h>
#include <gtest/gtest.h>
#include <array>
#include <cmath>
#include <cstdint>
#include <future>
#include <memory>
#include <random>
#include <unordered_map>

#define AES_CIRCUIT_TEST_FRIENDS                                     \
  friend class AesCircuitSBoxTestSuite;                              \
  FRIEND_TEST(AesCircuitSBoxTestSuite, SBoxInplaceTest);             \
  FRIEND_TEST(AesCircuitSBoxTestSuite, InverseSBoxInplaceTest);      \
  friend class AesCircuitMixColumnsTestSuite;                        \
  FRIEND_TEST(AesCircuitMixColumnsTestSuite, MixColumnsInplaceTest); \
  FRIEND_TEST(AesCircuitMixColumnsTestSuite, InverseMixColumnsInplaceTest);

#include "fbpcf/engine/communication/test/AgentFactoryCreationHelper.h"
#include "fbpcf/engine/util/aes.h"
#include "fbpcf/mpc_std_lib/aes_circuit/AesCircuit.h"
#include "fbpcf/mpc_std_lib/aes_circuit/AesCircuitCtr.h"
#include "fbpcf/mpc_std_lib/aes_circuit/AesCircuitCtrFactory.h"
#include "fbpcf/mpc_std_lib/aes_circuit/AesCircuitCtr_impl.h"
#include "fbpcf/mpc_std_lib/aes_circuit/AesCircuitFactory.h"
#include "fbpcf/mpc_std_lib/aes_circuit/AesCircuit_impl.h"
#include "fbpcf/mpc_std_lib/aes_circuit/DummyAesCircuitFactory.h"
#include "fbpcf/mpc_std_lib/aes_circuit/IAesCircuit.h"
#include "fbpcf/mpc_std_lib/aes_circuit/IAesCircuitCtr.h"
#include "fbpcf/mpc_std_lib/util/test/util.h"
#include "fbpcf/mpc_std_lib/util/util.h"
#include "fbpcf/scheduler/SchedulerHelper.h"
#include "fbpcf/test/TestHelper.h"

namespace fbpcf::mpc_std_lib::aes_circuit {

template <typename BitType>
class AesCircuitTests : public AesCircuit<BitType> {
 public:
  void testShiftRowInPlace(std::vector<bool> plaintext) {
    std::array<std::array<std::array<bool, 8>, 4>, 4> block;
    for (int k = 0; k < 4; ++k) {
      for (int i = 0; i < 4; i++) {
        for (int j = 0; j < 8; j++) {
          block[k][i][j] = plaintext[32 * k + 8 * i + j];
        }
      }
    }

    AesCircuit<bool>::shiftRowInPlace(block);
    for (int k = 0; k < 4; ++k) {
      for (int i = 0; i < 4; i++) {
        for (int j = 0; j < 8; j++) {
          EXPECT_EQ(block[k][i][j], plaintext[32 * ((k + i) % 4) + 8 * i + j]);
        }
      }
    }
  }

  void testWordConversion() {
    using ByteType = std::array<bool, 8>;
    using WordType = std::array<ByteType, 4>;
    std::vector<bool> input;
    size_t blockNo = 5;
    size_t blockSize = 128;
    for (int i = 0; i < blockSize * blockNo; ++i) {
      input.push_back((i % 2) == 0);
    }
    std::vector<std::array<WordType, 4>> expectedWords;
    ByteType sampleByte = {true, false, true, false, true, false, true, false};
    WordType sampleWord = {sampleByte, sampleByte, sampleByte, sampleByte};
    std::array<WordType, 4> sampleBlock = {
        sampleWord, sampleWord, sampleWord, sampleWord};
    for (int i = 0; i < blockNo; ++i) {
      expectedWords.push_back(sampleBlock);
    }
    auto convertedWords = AesCircuit<bool>::convertToWords(input);
    testVectorEq(convertedWords, expectedWords);

    auto convertedBits = AesCircuit<bool>::convertFromWords(convertedWords);
    testVectorEq(convertedBits, input);
  }
};

void intToBinaryArray(unsigned int n, std::array<bool, 8>& array) {
  for (unsigned int i = 0; i != 8; ++i) {
    array[7 - i] = n & 1;
    n = n >> 1;
  }
}

void int8VecToBinaryVec(
    std::vector<uint8_t>& intVec,
    std::vector<bool>& BinaryVec) {
  for (auto intUnit : intVec) {
    for (size_t i = 0; i < 8; ++i) {
      BinaryVec.push_back((intUnit >> (7 - i) & 1) == 1);
    }
  }
}

void loadValueToLocalAes(
    std::vector<uint8_t>& value,
    std::vector<__m128i>& aesStore) {
  size_t blockNo = value.size() / 16;
  for (int i = 0; i < blockNo; ++i) {
    uint8_t tmparray[16];
    for (int j = 0; j < 16; ++j) {
      tmparray[j] = value[i * 16 + j];
    }
    __m128i tmp = _mm_loadu_si128((const __m128i*)tmparray);
    aesStore.push_back(tmp);
  }
}

void loadValueFromLocalAes(__m128i aesStore, std::vector<uint8_t>& memStore) {
  uint8_t tmparray[16];
  _mm_storeu_si128((__m128i*)tmparray, aesStore);
  memStore.insert(memStore.end(), &tmparray[0], &tmparray[16]);
}

std::vector<bool> generateRandomPlaintext(int size = 128) {
  std::random_device rd;
  std::mt19937_64 e(rd());
  std::uniform_int_distribution<uint8_t> dist(0, 1);

  std::vector<bool> plaintext(size);
  for (size_t i = 0; i < size; i++) {
    plaintext[i] = dist(e);
  }
  return plaintext;
}

void testDummyAesCircuitInPlaintext(
    std::shared_ptr<insecure::DummyAesCircuitFactory<bool>>
        DummyAesCircuitFactory) {
  auto DummyAesCircuit = DummyAesCircuitFactory->create();
  auto plaintext = generateRandomPlaintext();
  std::vector<bool> dummyKey(11 * 16 * 8);
  auto rst1 = DummyAesCircuit->encrypt(plaintext, dummyKey);
  testVectorEq(rst1, plaintext);
  auto rst2 = DummyAesCircuit->decrypt(rst1, dummyKey);
  testVectorEq(rst2, rst1);
}

TEST(AesCircuitTest, testDummyAesCircuit) {
  testDummyAesCircuitInPlaintext(
      std::make_unique<insecure::DummyAesCircuitFactory<bool>>());
}

TEST(AesCircuitTest, testShiftRowInPlace) {
  auto plaintext = generateRandomPlaintext();
  AesCircuitTests<bool> test;
  test.testShiftRowInPlace(plaintext);
}

TEST(AesCircuitTest, testWordConversion) {
  AesCircuitTests<bool> test;
  test.testWordConversion();
}

class AesCircuitSBoxTestSuite
    : public ::testing::TestWithParam<std::tuple<uint8_t, uint8_t>> {};

TEST_P(AesCircuitSBoxTestSuite, SBoxInplaceTest) {
  AesCircuit<bool> aes;
  std::array<bool, 8> result;
  std::array<bool, 8> expectedResult;
  uint8_t input = std::get<0>(GetParam());
  uint8_t expectedOutput = std::get<1>(GetParam());
  intToBinaryArray(input, result);
  intToBinaryArray(expectedOutput, expectedResult);

  aes.sBoxInPlace(result);

  testArrayEq(expectedResult, result);
}

TEST_P(AesCircuitSBoxTestSuite, InverseSBoxInplaceTest) {
  AesCircuit<bool> aes;
  std::array<bool, 8> result;
  std::array<bool, 8> expectedResult;
  uint8_t input = std::get<1>(GetParam());
  uint8_t expectedOutput = std::get<0>(GetParam());
  intToBinaryArray(input, result);
  intToBinaryArray(expectedOutput, expectedResult);

  aes.inverseSBoxInPlace(result);

  testArrayEq(expectedResult, result);
}

INSTANTIATE_TEST_SUITE_P(
    SBoxTests,
    AesCircuitSBoxTestSuite,
    ::testing::Values(
        std::make_tuple(0x00, 0x63),
        std::make_tuple(0x11, 0x82),
        std::make_tuple(0x22, 0x93),
        std::make_tuple(0x33, 0xC3),
        std::make_tuple(0x44, 0x1B),
        std::make_tuple(0x55, 0xFC),
        std::make_tuple(0x66, 0x33),
        std::make_tuple(0x77, 0xF5),
        std::make_tuple(0x88, 0xC4),
        std::make_tuple(0x99, 0xEE),
        std::make_tuple(0xAA, 0xAC),
        std::make_tuple(0xBB, 0xEA),
        std::make_tuple(0xCC, 0x4B),
        std::make_tuple(0xDD, 0xC1),
        std::make_tuple(0xEE, 0x28),
        std::make_tuple(0xFF, 0x16)));

class AesCircuitMixColumnsTestSuite
    : public ::testing::TestWithParam<std::tuple<
          uint8_t,
          uint8_t,
          uint8_t,
          uint8_t,
          uint8_t,
          uint8_t,
          uint8_t,
          uint8_t>> {};

TEST_P(AesCircuitMixColumnsTestSuite, MixColumnsInplaceTest) {
  AesCircuit<bool> aes;
  std::array<std::array<bool, 8>, 4> result;
  std::array<std::uint8_t, 4> input;
  std::array<std::array<bool, 8>, 4> expectedResult;
  std::array<std::uint8_t, 4> expectedOutput;

  input[0] = std::get<0>(GetParam());
  input[1] = std::get<1>(GetParam());
  input[2] = std::get<2>(GetParam());
  input[3] = std::get<3>(GetParam());

  expectedOutput[0] = std::get<4>(GetParam());
  expectedOutput[1] = std::get<5>(GetParam());
  expectedOutput[2] = std::get<6>(GetParam());
  expectedOutput[3] = std::get<7>(GetParam());

  for (unsigned int i = 0; i < 4; i++) {
    intToBinaryArray(input[i], result[i]);
    intToBinaryArray(expectedOutput[i], expectedResult[i]);
  }

  aes.mixColumnsInPlace(result);

  testArrayEq(expectedResult, result);
}

TEST_P(AesCircuitMixColumnsTestSuite, InverseMixColumnsInplaceTest) {
  AesCircuit<bool> aes;
  std::array<std::array<bool, 8>, 4> result;
  std::array<std::uint8_t, 4> input;
  std::array<std::array<bool, 8>, 4> expectedResult;
  std::array<std::uint8_t, 4> expectedOutput;

  input[0] = std::get<4>(GetParam());
  input[1] = std::get<5>(GetParam());
  input[2] = std::get<6>(GetParam());
  input[3] = std::get<7>(GetParam());

  expectedOutput[0] = std::get<0>(GetParam());
  expectedOutput[1] = std::get<1>(GetParam());
  expectedOutput[2] = std::get<2>(GetParam());
  expectedOutput[3] = std::get<3>(GetParam());

  for (unsigned int i = 0; i < 4; i++) {
    intToBinaryArray(input[i], result[i]);
    intToBinaryArray(expectedOutput[i], expectedResult[i]);
  }

  aes.inverseMixColumnsInPlace(result);

  testArrayEq(expectedResult, result);
}

INSTANTIATE_TEST_SUITE_P(
    MixColumnsTests,
    AesCircuitMixColumnsTestSuite,
    ::testing::Values(
        std::make_tuple(0xdb, 0x13, 0x53, 0x45, 0x8e, 0x4d, 0xa1, 0xbc),
        std::make_tuple(0xF2, 0x0A, 0x22, 0x5C, 0x9F, 0xDC, 0x58, 0x9D),
        std::make_tuple(0x01, 0x01, 0x01, 0x01, 0x01, 0x01, 0x01, 0x01),
        std::make_tuple(0xc6, 0xc6, 0xc6, 0xc6, 0xc6, 0xc6, 0xc6, 0xc6),
        std::make_tuple(0xd4, 0xd4, 0xd4, 0xd5, 0xd5, 0xd5, 0xd7, 0xd6),
        std::make_tuple(0x2d, 0x26, 0x31, 0x4c, 0x4d, 0x7e, 0xbd, 0xf8)));

void testAesCircuitEncrypt(
    std::shared_ptr<AesCircuitFactory<bool>> AesCircuitFactory) {
  auto AesCircuit = AesCircuitFactory->create();

  std::random_device rd;
  std::mt19937_64 e(rd());
  std::uniform_int_distribution<uint8_t> dist(0, 0xFF);
  size_t blockNo = dist(e);

  // generate random key
  __m128i key = _mm_set_epi32(dist(e), dist(e), dist(e), dist(e));
  // generate random plaintext
  std::vector<uint8_t> plaintext;
  plaintext.reserve(blockNo * 16);
  for (int i = 0; i < blockNo * 16; ++i) {
    plaintext.push_back(dist(e));
  }
  std::vector<__m128i> plaintextAES;
  loadValueToLocalAes(plaintext, plaintextAES);

  // expend key
  engine::util::Aes truthAes(key);
  auto expendedKey = truthAes.expandEncryptionKey(key);
  // extract key and plaintext
  std::vector<uint8_t> extractedKeys;
  extractedKeys.reserve(176);
  for (auto keyb : expendedKey) {
    loadValueFromLocalAes(keyb, extractedKeys);
  }

  // convert key and plaintext into bool vector
  std::vector<bool> keyBits;
  keyBits.reserve(1408);
  int8VecToBinaryVec(extractedKeys, keyBits);
  std::vector<bool> plaintextBits;
  plaintextBits.reserve(blockNo * 128);
  int8VecToBinaryVec(plaintext, plaintextBits);

  // encrypt in real aes and aes circuit
  truthAes.encryptInPlace(plaintextAES);
  auto ciphertextBits = AesCircuit->encrypt(plaintextBits, keyBits);

  // extract ciphertext
  std::vector<uint8_t> ciphertextTruth;
  ciphertextTruth.reserve(blockNo * 16);
  for (auto b : plaintextAES) {
    loadValueFromLocalAes(b, ciphertextTruth);
  }

  std::vector<bool> cipherextBitsTruth;
  cipherextBitsTruth.reserve(blockNo * 128);
  int8VecToBinaryVec(ciphertextTruth, cipherextBitsTruth);

  testVectorEq(ciphertextBits, cipherextBitsTruth);
}

TEST(AesCircuitTest, testAesCircuitEncrypt) {
  testAesCircuitEncrypt(std::make_unique<AesCircuitFactory<bool>>());
}

void testAesCircuitCtr(
    std::shared_ptr<AesCircuitCtrFactory<bool>> AesCircuitCtrFactory) {
  auto AesCircuitCtr = AesCircuitCtrFactory->create();

  std::random_device rd;
  std::mt19937_64 e(rd());
  std::uniform_int_distribution<uint8_t> dist(0, 0xFF);
  size_t blockNo = dist(e);

  // generate random key
  __m128i key = _mm_set_epi32(dist(e), dist(e), dist(e), dist(e));
  // generate random plaintext
  std::vector<uint8_t> plaintext;
  plaintext.reserve(blockNo * 16);
  for (int i = 0; i < blockNo * 16; ++i) {
    plaintext.push_back(dist(e));
  }

  // generate random mask
  std::vector<uint8_t> mask;
  mask.reserve(blockNo * 16);
  for (int i = 0; i < blockNo * 16; ++i) {
    mask.push_back(i);
  }

  // expend key
  engine::util::Aes truthAes(key);
  auto expendedKey = truthAes.expandEncryptionKey(key);
  // extract key and plaintext
  std::vector<uint8_t> extractedKeys;
  extractedKeys.reserve(176);
  for (auto keyb : expendedKey) {
    loadValueFromLocalAes(keyb, extractedKeys);
  }

  // convert key, plaintext, initial vector, and counter into bool vector
  std::vector<bool> keyBits;
  keyBits.reserve(1408);

  int8VecToBinaryVec(extractedKeys, keyBits);
  std::vector<bool> plaintextBits;
  plaintextBits.reserve(blockNo * 128);

  int8VecToBinaryVec(plaintext, plaintextBits);
  std::vector<bool> maskBits;
  maskBits.reserve(blockNo * 128);
  int8VecToBinaryVec(mask, maskBits);

  // encrypt in aes ctr circuit
  auto ciphertextBits =
      AesCircuitCtr->encrypt(plaintextBits, keyBits, maskBits);
  // encrypt in aes ctr circuit
  auto decryptedBits =
      AesCircuitCtr->decrypt(ciphertextBits, keyBits, maskBits);

  testVectorEq(decryptedBits, plaintextBits);
}

TEST(AesCircuitTest, testAesCircuitCtr) {
  testAesCircuitCtr(std::make_unique<AesCircuitCtrFactory<bool>>());
}

} // namespace fbpcf::mpc_std_lib::aes_circuit
