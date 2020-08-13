// Copyright 2020 Google LLC
//
// Licensed under the Apache License, Version 2.0 (the "License");
// you may not use this file except in compliance with the License.
// You may obtain a copy of the License at
//
//      http://www.apache.org/licenses/LICENSE-2.0
//
// Unless required by applicable law or agreed to in writing, software
// distributed under the License is distributed on an "AS IS" BASIS,
// WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
// See the License for the specific language governing permissions and
// limitations under the License.

#include "xls/ir/node_util.h"

#include <memory>

#include "gmock/gmock.h"
#include "gtest/gtest.h"
#include "absl/memory/memory.h"
#include "absl/strings/str_format.h"
#include "xls/common/status/matchers.h"
#include "xls/common/status/ret_check.h"
#include "xls/common/status/status_macros.h"
#include "xls/ir/function_builder.h"
#include "xls/ir/ir_test_base.h"
#include "xls/ir/op.h"
#include "xls/ir/package.h"

namespace xls {
namespace {

using status_testing::StatusIs;
using ::testing::HasSubstr;

class Result {
 public:
  Result(int64 leading_zero_count, int64 set_bit_count,
         int64 trailing_zero_count)
      : leading_zero_count_(leading_zero_count),
        set_bit_count_(set_bit_count),
        trailing_zero_count_(trailing_zero_count) {}

  bool operator==(const Result& other) const {
    return leading_zero_count_ == other.leading_zero_count_ &&
           set_bit_count_ == other.set_bit_count_ &&
           trailing_zero_count_ == other.trailing_zero_count_;
  }

 private:
  friend std::ostream& operator<<(std::ostream&, const Result&);

  int64 leading_zero_count_;
  int64 set_bit_count_;
  int64 trailing_zero_count_;
};

std::ostream& operator<<(std::ostream& os, const Result& result) {
  os << absl::StreamFormat("{%d, %d, %d}", result.leading_zero_count_,
                           result.set_bit_count_, result.trailing_zero_count_);
  return os;
}

class NodeUtilTest : public IrTestBase {
 protected:
  xabsl::StatusOr<Result> RunOn(const Bits& bits) {
    auto p = CreatePackage();
    FunctionBuilder fb("f", p.get());
    fb.Literal(bits);
    XLS_ASSIGN_OR_RETURN(Function * f, fb.Build());
    Node* n = f->return_value();
    int64 leading_zero_count, set_bit_count, trailing_zero_count;
    XLS_RET_CHECK(IsLiteralWithRunOfSetBits(
        n, &leading_zero_count, &set_bit_count, &trailing_zero_count));
    return Result{leading_zero_count, set_bit_count, trailing_zero_count};
  }
};

TEST_F(NodeUtilTest, RunOfSetBits) {
  Bits bits = UBits(0x0ff0, /*bit_count=*/16);
  XLS_ASSERT_OK_AND_ASSIGN(Result t, RunOn(bits));
  EXPECT_EQ(Result(4, 8, 4), t);

  bits = UBits(0x00ff, /*bit_count=*/16);
  XLS_ASSERT_OK_AND_ASSIGN(t, RunOn(bits));
  EXPECT_EQ(Result(8, 8, 0), t);

  bits = UBits(0x0500, /*bit_count=*/16);
  EXPECT_THAT(RunOn(bits),
              status_testing::StatusIs(absl::StatusCode::kInternal));

  bits = UBits(0x0010, /*bit_count=*/16);
  XLS_ASSERT_OK_AND_ASSIGN(t, RunOn(bits));
  EXPECT_EQ(Result(11, 1, 4), t);
}

TEST_F(NodeUtilTest, GatherBits) {
  auto p = CreatePackage();
  FunctionBuilder fb(TestName(), p.get());
  fb.Param("x", p->GetBitsType(8));
  XLS_ASSERT_OK_AND_ASSIGN(Function * f, fb.Build());
  XLS_ASSERT_OK_AND_ASSIGN(Node * gathered,
                           GatherBits(f->return_value(), {0, 2, 3, 4, 6}));
  f->set_return_value(gathered);
  EXPECT_EQ(p->DumpIr(), R"(package GatherBits

fn GatherBits(x: bits[8]) -> bits[5] {
  bit_slice.4: bits[1] = bit_slice(x, start=6, width=1)
  bit_slice.3: bits[3] = bit_slice(x, start=2, width=3)
  bit_slice.2: bits[1] = bit_slice(x, start=0, width=1)
  ret concat.5: bits[5] = concat(bit_slice.4, bit_slice.3, bit_slice.2)
}
)");
}

TEST_F(NodeUtilTest, GatherNoBits) {
  auto p = CreatePackage();
  FunctionBuilder fb(TestName(), p.get());
  fb.Param("x", p->GetBitsType(8));
  XLS_ASSERT_OK_AND_ASSIGN(Function * f, fb.Build());
  XLS_ASSERT_OK_AND_ASSIGN(Node * gathered, GatherBits(f->return_value(), {}));
  f->set_return_value(gathered);
  EXPECT_EQ(p->DumpIr(), R"(package GatherNoBits

fn GatherNoBits(x: bits[8]) -> bits[0] {
  ret literal.2: bits[0] = literal(value=0)
}
)");
}

TEST_F(NodeUtilTest, GatherAllTheBits) {
  auto p = CreatePackage();
  FunctionBuilder fb(TestName(), p.get());
  fb.Param("x", p->GetBitsType(8));
  XLS_ASSERT_OK_AND_ASSIGN(Function * f, fb.Build());
  XLS_ASSERT_OK_AND_ASSIGN(
      Node * gathered, GatherBits(f->return_value(), {0, 1, 2, 3, 4, 5, 6, 7}));
  f->set_return_value(gathered);
  EXPECT_EQ(p->DumpIr(), R"(package GatherAllTheBits

fn GatherAllTheBits(x: bits[8]) -> bits[8] {
  ret param.1: bits[8] = param(name=x)
}
)");
}

TEST_F(NodeUtilTest, GatherBitsIndicesNotSorted) {
  auto p = CreatePackage();
  FunctionBuilder fb(TestName(), p.get());
  fb.Param("x", p->GetBitsType(8));
  XLS_ASSERT_OK_AND_ASSIGN(Function * f, fb.Build());
  EXPECT_THAT(GatherBits(f->return_value(), {0, 6, 3}),
              StatusIs(absl::StatusCode::kInternal,
                       testing::HasSubstr("Gather indices not sorted.")));
}

TEST_F(NodeUtilTest, GatherBitsIndicesNotUnique) {
  auto p = CreatePackage();
  FunctionBuilder fb(TestName(), p.get());
  fb.Param("x", p->GetBitsType(8));
  XLS_ASSERT_OK_AND_ASSIGN(Function * f, fb.Build());
  EXPECT_THAT(GatherBits(f->return_value(), {0, 2, 2}),
              StatusIs(absl::StatusCode::kInternal,
                       testing::HasSubstr("Gather indices not unique.")));
}

TEST_F(NodeUtilTest, IsLiteralMask) {
  auto p = CreatePackage();
  FunctionBuilder fb(TestName(), p.get());
  auto seven_3b = fb.Literal(UBits(0b111, 3));
  auto two_3b = fb.Literal(UBits(0b011, 3));
  auto one_1b = fb.Literal(UBits(0b1, 1));
  auto zero_1b = fb.Literal(UBits(0b0, 1));
  auto zero_0b = fb.Literal(UBits(0b0, 0));

  int64 leading_zeros, trailing_ones;
  EXPECT_TRUE(IsLiteralMask(seven_3b.node(), &leading_zeros, &trailing_ones));
  EXPECT_EQ(0, leading_zeros);
  EXPECT_EQ(3, trailing_ones);

  EXPECT_TRUE(IsLiteralMask(two_3b.node(), &leading_zeros, &trailing_ones));
  EXPECT_EQ(1, leading_zeros);
  EXPECT_EQ(2, trailing_ones);

  EXPECT_TRUE(IsLiteralMask(one_1b.node(), &leading_zeros, &trailing_ones));
  EXPECT_EQ(0, leading_zeros);
  EXPECT_EQ(1, trailing_ones);

  EXPECT_FALSE(IsLiteralMask(zero_1b.node(), &leading_zeros, &trailing_ones));
  EXPECT_FALSE(IsLiteralMask(zero_0b.node(), &leading_zeros, &trailing_ones));
}

TEST_F(NodeUtilTest, NonReductiveEquivalents) {
  XLS_ASSERT_OK_AND_ASSIGN(
      Op and_op, OpToNonReductionOp(Op::kAndReduce));
  EXPECT_EQ(and_op, Op::kAnd);
  XLS_ASSERT_OK_AND_ASSIGN(
      Op or_op, OpToNonReductionOp(Op::kOrReduce));
  EXPECT_EQ(or_op, Op::kOr);
  XLS_ASSERT_OK_AND_ASSIGN(
      Op xor_op, OpToNonReductionOp(Op::kXorReduce));
  EXPECT_EQ(xor_op, Op::kXor);
  EXPECT_FALSE(OpToNonReductionOp(Op::kBitSlice).ok());
}

}  // namespace
}  // namespace xls
