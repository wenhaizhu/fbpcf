/*
 * Copyright (c) Meta Platforms, Inc. and affiliates.
 *
 * This source code is licensed under the MIT license found in the
 * LICENSE file in the root directory of this source tree.
 */

#include <gmock/gmock.h>
#include <gtest/gtest.h>
#include <random>
#include <stdexcept>

#include "fbpcf/frontend/BitString.h"
#include "fbpcf/scheduler/PlaintextScheduler.h"
#include "fbpcf/scheduler/WireKeeper.h"
#include "fbpcf/test/TestHelper.h"

namespace fbpcf::frontend {
TEST(StringTest, testInputAndOutput) {
  std::random_device rd;
  std::mt19937_64 e(rd());
  std::uniform_int_distribution<int64_t> dSize(1, 1024);

  std::uniform_int_distribution<uint8_t> dBool(0, 1);

  scheduler::SchedulerKeeper<0>::setScheduler(
      std::make_unique<scheduler::PlaintextScheduler>(
          scheduler::WireKeeper::createWithUnorderedMap()));
  using SecString = BitString<true, 0>;
  using PubString = BitString<false, 0>;
  using SecBatchString = BitString<true, 0, true>;
  using PubBatchString = BitString<false, 0, true>;

  std::vector<bool> testValue(dSize(e));
  for (size_t i = 0; i < testValue.size(); i++) {
    testValue[i] = dBool(e);
  }
  std::vector<std::vector<bool>> testBatchValue(
      dSize(e), std::vector<bool>(dSize(e)));
  for (size_t i = 0; i < testBatchValue.size(); i++) {
    for (size_t j = 0; j < testBatchValue.at(0).size(); j++) {
      testBatchValue[i][j] = dBool(e);
    }
  }

  SecString v1(testValue, 0);
  PubString v2(testValue);

  SecBatchString v3(testBatchValue, 0);
  PubBatchString v4(testBatchValue);

  SecString v5(v1.extractStringShare());
  SecBatchString v6(v3.extractStringShare());

  auto t1 = v1.openToParty(0).getValue();
  auto t2 = v2.getValue();
  auto t3 = v3.openToParty(0).getValue();
  auto t4 = v4.getValue();
  auto t5 = v5.openToParty(0).getValue();
  auto t6 = v6.openToParty(0).getValue();
  testVectorEq(t1, testValue);
  testVectorEq(t2, testValue);
  testVectorEq(t5, testValue);
  for (size_t i = 0; i < testBatchValue.size(); i++) {
    testVectorEq(t3.at(i), testBatchValue.at(i));
    testVectorEq(t4.at(i), testBatchValue.at(i));
    testVectorEq(t6.at(i), testBatchValue.at(i));
  }
}

TEST(StringTest, testNOT) {
  std::random_device rd;
  std::mt19937_64 e(rd());
  std::uniform_int_distribution<int64_t> dSize(1, 1024);

  std::uniform_int_distribution<uint8_t> dBool(0, 1);

  scheduler::SchedulerKeeper<0>::setScheduler(
      std::make_unique<scheduler::PlaintextScheduler>(
          scheduler::WireKeeper::createWithUnorderedMap()));
  using SecString = BitString<true, 0>;
  using PubString = BitString<false, 0>;
  using SecBatchString = BitString<true, 0, true>;
  using PubBatchString = BitString<false, 0, true>;

  std::vector<bool> testValue(dSize(e));
  for (size_t i = 0; i < testValue.size(); i++) {
    testValue[i] = dBool(e);
  }
  std::vector<std::vector<bool>> testBatchValue(
      dSize(e), std::vector<bool>(dSize(e)));
  for (size_t i = 0; i < testBatchValue.size(); i++) {
    for (size_t j = 0; j < testBatchValue.at(0).size(); j++) {
      testBatchValue[i][j] = dBool(e);
    }
  }

  SecString v1(testValue, 0);
  PubString v2(testValue);

  SecBatchString v3(testBatchValue, 0);
  PubBatchString v4(testBatchValue);

  auto t1 = (!v1).openToParty(0).getValue();
  auto t2 = (!v2).getValue();
  auto t3 = (!v3).openToParty(0).getValue();
  auto t4 = (!v4).getValue();

  for (size_t i = 0; i < testValue.size(); i++) {
    testValue[i] = !testValue[i];
  }
  for (size_t i = 0; i < testBatchValue.size(); i++) {
    for (size_t j = 0; j < testBatchValue.at(0).size(); j++) {
      testBatchValue[i][j] = !testBatchValue[i][j];
    }
  }

  testVectorEq(t1, testValue);
  testVectorEq(t2, testValue);

  for (size_t i = 0; i < testBatchValue.size(); i++) {
    testVectorEq(t3.at(i), testBatchValue.at(i));
    testVectorEq(t4.at(i), testBatchValue.at(i));
  }
}

TEST(StringTest, testAND) {
  std::random_device rd;
  std::mt19937_64 e(rd());
  std::uniform_int_distribution<int64_t> dSize(1, 1024);

  std::uniform_int_distribution<uint8_t> dBool(0, 1);

  scheduler::SchedulerKeeper<0>::setScheduler(
      std::make_unique<scheduler::PlaintextScheduler>(
          scheduler::WireKeeper::createWithUnorderedMap()));
  using SecString = BitString<true, 0>;
  using PubString = BitString<false, 0>;
  using SecBatchString = BitString<true, 0, true>;
  using PubBatchString = BitString<false, 0, true>;

  std::vector<bool> testValue1(dSize(e));
  auto testValue2 = testValue1;
  for (size_t i = 0; i < testValue1.size(); i++) {
    testValue1[i] = dBool(e);
    testValue2[i] = dBool(e);
  }
  std::vector<std::vector<bool>> testBatchValue1(
      dSize(e), std::vector<bool>(dSize(e)));
  auto testBatchValue2 = testBatchValue1;
  for (size_t i = 0; i < testBatchValue1.size(); i++) {
    for (size_t j = 0; j < testBatchValue1.at(0).size(); j++) {
      testBatchValue1[i][j] = dBool(e);
      testBatchValue2[i][j] = dBool(e);
    }
  }

  SecString v1(testValue1, 0);
  PubString v2(testValue1);
  SecString u1(testValue2, 0);
  PubString u2(testValue2);

  SecBatchString v3(testBatchValue1, 0);
  PubBatchString v4(testBatchValue1);
  SecBatchString u3(testBatchValue2, 0);
  PubBatchString u4(testBatchValue2);

  auto t1 = (v1 & u1).openToParty(0).getValue();
  auto t2 = (v1 & u2).openToParty(0).getValue();
  auto t3 = (v2 & u1).openToParty(0).getValue();
  auto t4 = (v2 & u2).getValue();

  auto t5 = (v3 & u3).openToParty(0).getValue();
  auto t6 = (v3 & u4).openToParty(0).getValue();
  auto t7 = (v4 & u3).openToParty(0).getValue();
  auto t8 = (v4 & u4).getValue();

  for (size_t i = 0; i < testValue1.size(); i++) {
    testValue1[i] = testValue1[i] & testValue2[i];
  }
  for (size_t i = 0; i < testBatchValue1.size(); i++) {
    for (size_t j = 0; j < testBatchValue1.at(0).size(); j++) {
      testBatchValue1[i][j] = testBatchValue1[i][j] & testBatchValue2[i][j];
    }
  }

  testVectorEq(t1, testValue1);
  testVectorEq(t2, testValue1);
  testVectorEq(t3, testValue1);
  testVectorEq(t4, testValue1);

  for (size_t i = 0; i < testBatchValue1.size(); i++) {
    testVectorEq(t5.at(i), testBatchValue1.at(i));
    testVectorEq(t6.at(i), testBatchValue1.at(i));
    testVectorEq(t7.at(i), testBatchValue1.at(i));
    testVectorEq(t8.at(i), testBatchValue1.at(i));
  }
}

TEST(StringTest, testXOR) {
  std::random_device rd;
  std::mt19937_64 e(rd());
  std::uniform_int_distribution<int64_t> dSize(1, 1024);

  std::uniform_int_distribution<uint8_t> dBool(0, 1);

  scheduler::SchedulerKeeper<0>::setScheduler(
      std::make_unique<scheduler::PlaintextScheduler>(
          scheduler::WireKeeper::createWithUnorderedMap()));
  using SecString = BitString<true, 0>;
  using PubString = BitString<false, 0>;
  using SecBatchString = BitString<true, 0, true>;
  using PubBatchString = BitString<false, 0, true>;

  std::vector<bool> testValue1(dSize(e));
  auto testValue2 = testValue1;
  for (size_t i = 0; i < testValue1.size(); i++) {
    testValue1[i] = dBool(e);
    testValue2[i] = dBool(e);
  }
  std::vector<std::vector<bool>> testBatchValue1(
      dSize(e), std::vector<bool>(dSize(e)));
  auto testBatchValue2 = testBatchValue1;
  for (size_t i = 0; i < testBatchValue1.size(); i++) {
    for (size_t j = 0; j < testBatchValue1.at(0).size(); j++) {
      testBatchValue1[i][j] = dBool(e);
      testBatchValue2[i][j] = dBool(e);
    }
  }

  SecString v1(testValue1, 0);
  PubString v2(testValue1);
  SecString u1(testValue2, 0);
  PubString u2(testValue2);

  SecBatchString v3(testBatchValue1, 0);
  PubBatchString v4(testBatchValue1);
  SecBatchString u3(testBatchValue2, 0);
  PubBatchString u4(testBatchValue2);

  auto t1 = (v1 ^ u1).openToParty(0).getValue();
  auto t2 = (v1 ^ u2).openToParty(0).getValue();
  auto t3 = (v2 ^ u1).openToParty(0).getValue();
  auto t4 = (v2 ^ u2).getValue();

  auto t5 = (v3 ^ u3).openToParty(0).getValue();
  auto t6 = (v3 ^ u4).openToParty(0).getValue();
  auto t7 = (v4 ^ u3).openToParty(0).getValue();
  auto t8 = (v4 ^ u4).getValue();

  for (size_t i = 0; i < testValue1.size(); i++) {
    testValue1[i] = testValue1[i] ^ testValue2[i];
  }
  for (size_t i = 0; i < testBatchValue1.size(); i++) {
    for (size_t j = 0; j < testBatchValue1.at(0).size(); j++) {
      testBatchValue1[i][j] = testBatchValue1[i][j] ^ testBatchValue2[i][j];
    }
  }

  testVectorEq(t1, testValue1);
  testVectorEq(t2, testValue1);
  testVectorEq(t3, testValue1);
  testVectorEq(t4, testValue1);

  for (size_t i = 0; i < testBatchValue1.size(); i++) {
    testVectorEq(t5.at(i), testBatchValue1.at(i));
    testVectorEq(t6.at(i), testBatchValue1.at(i));
    testVectorEq(t7.at(i), testBatchValue1.at(i));
    testVectorEq(t8.at(i), testBatchValue1.at(i));
  }
}

TEST(StringTest, testMux) {
  std::random_device rd;
  std::mt19937_64 e(rd());
  std::uniform_int_distribution<int64_t> dSize(1, 1024);

  std::uniform_int_distribution<uint8_t> dBool(0, 1);

  scheduler::SchedulerKeeper<0>::setScheduler(
      std::make_unique<scheduler::PlaintextScheduler>(
          scheduler::WireKeeper::createWithUnorderedMap()));
  using SecString = BitString<true, 0>;
  using PubString = BitString<false, 0>;
  using SecBatchString = BitString<true, 0, true>;
  using PubBatchString = BitString<false, 0, true>;

  std::vector<bool> testValue1(dSize(e));
  bool testChoice = dBool(e);
  auto testValue2 = testValue1;
  for (size_t i = 0; i < testValue1.size(); i++) {
    testValue1[i] = dBool(e);
    testValue2[i] = dBool(e);
  }
  std::vector<std::vector<bool>> testBatchValue1(
      dSize(e), std::vector<bool>(dSize(e)));
  std::vector<bool> testBatchChoice(testBatchValue1.at(0).size());
  auto testBatchValue2 = testBatchValue1;
  for (size_t i = 0; i < testBatchValue1.size(); i++) {
    for (size_t j = 0; j < testBatchValue1.at(0).size(); j++) {
      testBatchValue1[i][j] = dBool(e);
      testBatchValue2[i][j] = dBool(e);
    }
  }
  for (size_t j = 0; j < testBatchValue1.at(0).size(); j++) {
    testBatchChoice[j] = dBool(e);
  }

  SecString v1(testValue1, 0);
  PubString v2(testValue1);
  SecString u1(testValue2, 0);
  PubString u2(testValue2);

  Bit<true, 0> c1(testChoice, 0);
  Bit<false, 0> c2(testChoice);

  SecBatchString v3(testBatchValue1, 0);
  PubBatchString v4(testBatchValue1);
  SecBatchString u3(testBatchValue2, 0);
  PubBatchString u4(testBatchValue2);
  Bit<true, 0, true> c3(testBatchChoice, 0);
  Bit<false, 0, true> c4(testBatchChoice);

  auto t1 = v1.mux(c1, u1).openToParty(0).getValue();
  auto t2 = v1.mux(c2, u1).openToParty(0).getValue();
  auto t3 = v1.mux(c1, u2).openToParty(0).getValue();
  auto t4 = v1.mux(c2, u2).openToParty(0).getValue();
  auto t5 = v2.mux(c1, u1).openToParty(0).getValue();
  auto t6 = v2.mux(c2, u1).openToParty(0).getValue();
  auto t7 = v2.mux(c1, u2).openToParty(0).getValue();
  auto t8 = v2.mux(c2, u2).getValue();

  auto t9 = v3.mux(c3, u3).openToParty(0).getValue();
  auto t10 = v3.mux(c4, u3).openToParty(0).getValue();
  auto t11 = v3.mux(c3, u4).openToParty(0).getValue();
  auto t12 = v3.mux(c4, u4).openToParty(0).getValue();
  auto t13 = v4.mux(c3, u3).openToParty(0).getValue();
  auto t14 = v4.mux(c4, u3).openToParty(0).getValue();
  auto t15 = v4.mux(c3, u4).openToParty(0).getValue();
  auto t16 = v4.mux(c4, u4).getValue();

  for (size_t i = 0; i < testValue1.size(); i++) {
    testValue1[i] = testChoice ? testValue2[i] : testValue1[i];
  }
  for (size_t i = 0; i < testBatchValue1.size(); i++) {
    for (size_t j = 0; j < testBatchValue1.at(0).size(); j++) {
      testBatchValue1[i][j] =
          testBatchChoice[j] ? testBatchValue2[i][j] : testBatchValue1[i][j];
    }
  }

  testVectorEq(t1, testValue1);
  testVectorEq(t2, testValue1);
  testVectorEq(t3, testValue1);
  testVectorEq(t4, testValue1);
  testVectorEq(t5, testValue1);
  testVectorEq(t6, testValue1);
  testVectorEq(t7, testValue1);
  testVectorEq(t8, testValue1);

  for (size_t i = 0; i < testBatchValue1.size(); i++) {
    testVectorEq(t9.at(i), testBatchValue1.at(i));
    testVectorEq(t10.at(i), testBatchValue1.at(i));
    testVectorEq(t11.at(i), testBatchValue1.at(i));
    testVectorEq(t12.at(i), testBatchValue1.at(i));
    testVectorEq(t13.at(i), testBatchValue1.at(i));
    testVectorEq(t14.at(i), testBatchValue1.at(i));
    testVectorEq(t15.at(i), testBatchValue1.at(i));
    testVectorEq(t16.at(i), testBatchValue1.at(i));
  }
}

TEST(StringTest, testResizeWithAND) {
  std::random_device rd;
  std::mt19937_64 e(rd());
  std::uniform_int_distribution<int64_t> dSize(1, 1024);

  std::uniform_int_distribution<uint8_t> dBool(0, 1);

  scheduler::SchedulerKeeper<0>::setScheduler(
      std::make_unique<scheduler::PlaintextScheduler>(
          scheduler::WireKeeper::createWithUnorderedMap()));
  using SecString = BitString<true, 0>;
  using PubString = BitString<false, 0>;
  using SecBatchString = BitString<true, 0, true>;
  using PubBatchString = BitString<false, 0, true>;

  std::vector<bool> testValue1(dSize(e));
  auto testValue2 = testValue1;
  testValue2.resize(2 * testValue1.size());
  for (size_t i = 0; i < testValue1.size(); i++) {
    testValue1[i] = dBool(e);
    testValue2[i] = dBool(e);
    testValue2[i + testValue1.size()] = dBool(e);
  }
  std::vector<std::vector<bool>> testBatchValue1(
      dSize(e), std::vector<bool>(dSize(e)));
  auto testBatchValue2 = testBatchValue1;
  testBatchValue2.resize(testBatchValue1.size() * 2);
  for (size_t i = 0; i < testBatchValue1.size(); i++) {
    testBatchValue2[i + testBatchValue1.size()] =
        std::vector<bool>(testBatchValue1.at(0).size());
    for (size_t j = 0; j < testBatchValue1.at(0).size(); j++) {
      testBatchValue1[i][j] = dBool(e);
      testBatchValue2[i][j] = dBool(e);
      testBatchValue2[i + testBatchValue1.size()][j] = dBool(e);
    }
  }

  SecString v1(testValue1, 0);
  PubString v2(testValue1);
  SecString u1(testValue2, 0);
  PubString u2(testValue2);
  u1.resize(v1.size());
  u2.resize(v2.size());

  SecBatchString v3(testBatchValue1, 0);
  PubBatchString v4(testBatchValue1);
  SecBatchString u3(testBatchValue2, 0);
  PubBatchString u4(testBatchValue2);
  u3.resize(v3.size());
  u4.resize(v4.size());

  auto t1 = (v1 & u1).openToParty(0).getValue();
  auto t2 = (v1 & u2).openToParty(0).getValue();
  auto t3 = (v2 & u1).openToParty(0).getValue();
  auto t4 = (v2 & u2).getValue();

  auto t5 = (v3 & u3).openToParty(0).getValue();
  auto t6 = (v3 & u4).openToParty(0).getValue();
  auto t7 = (v4 & u3).openToParty(0).getValue();
  auto t8 = (v4 & u4).getValue();

  for (size_t i = 0; i < testValue1.size(); i++) {
    testValue1[i] = testValue1[i] & testValue2[i];
  }
  for (size_t i = 0; i < testBatchValue1.size(); i++) {
    for (size_t j = 0; j < testBatchValue1.at(0).size(); j++) {
      testBatchValue1[i][j] = testBatchValue1[i][j] & testBatchValue2[i][j];
    }
  }

  testVectorEq(t1, testValue1);
  testVectorEq(t2, testValue1);
  testVectorEq(t3, testValue1);
  testVectorEq(t4, testValue1);

  for (size_t i = 0; i < testBatchValue1.size(); i++) {
    testVectorEq(t5.at(i), testBatchValue1.at(i));
    testVectorEq(t6.at(i), testBatchValue1.at(i));
    testVectorEq(t7.at(i), testBatchValue1.at(i));
    testVectorEq(t8.at(i), testBatchValue1.at(i));
  }
}

} // namespace fbpcf::frontend
