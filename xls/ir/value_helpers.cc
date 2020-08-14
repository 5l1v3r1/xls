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

#include "xls/ir/value_helpers.h"

#include "xls/common/logging/logging.h"
#include "xls/common/status/status_macros.h"
#include "xls/ir/type.h"

namespace xls {
namespace {

Value ValueOfType(Type* type,
                  const std::function<Bits(int64 bit_count)>& fbits) {
  switch (type->kind()) {
    case TypeKind::kBits:
      return Value(fbits(type->AsBitsOrDie()->bit_count()));
    case TypeKind::kTuple: {
      std::vector<Value> elements;
      for (Type* element_type : type->AsTupleOrDie()->element_types()) {
        elements.push_back(ValueOfType(element_type, fbits));
      }
      return Value::Tuple(elements);
    }
    case TypeKind::kArray: {
      std::vector<Value> elements;
      for (int64 i = 0; i < type->AsArrayOrDie()->size(); ++i) {
        elements.push_back(
            ValueOfType(type->AsArrayOrDie()->element_type(), fbits));
      }
      return Value::Array(elements).value();
    }
    case TypeKind::kToken:
      break;
  }
  XLS_LOG(FATAL) << "Invalid kind: " << type->kind();
}

}  // namespace

Value ZeroOfType(Type* type) {
  return ValueOfType(
      type, [](int64 bit_count) { return UBits(0, /*bit_count=*/bit_count); });
}

Value AllOnesOfType(Type* type) {
  return ValueOfType(
      type, [](int64 bit_count) { return SBits(-1, /*bit_count=*/bit_count); });
}

Value RandomValue(Type* type, std::minstd_rand* engine) {
  if (type->IsTuple()) {
    TupleType* tuple_type = type->AsTupleOrDie();
    std::vector<Value> elements;
    for (int64 i = 0; i < tuple_type->size(); ++i) {
      elements.push_back(RandomValue(tuple_type->element_type(i), engine));
    }
    return Value::Tuple(elements);
  }
  if (type->IsArray()) {
    ArrayType* array_type = type->AsArrayOrDie();
    std::vector<Value> elements;
    for (int64 i = 0; i < array_type->size(); ++i) {
      elements.push_back(RandomValue(array_type->element_type(), engine));
    }
    return Value::Array(elements).value();
  }
  int64 bit_count = type->AsBitsOrDie()->bit_count();
  std::vector<uint8> bytes;
  std::uniform_int_distribution<uint8> generator(0, 255);
  for (int64 i = 0; i < bit_count; i += 8) {
    bytes.push_back(generator(*engine));
  }
  return Value(Bits::FromBytes(bytes, bit_count));
}

std::vector<Value> RandomFunctionArguments(Function* f,
                                           std::minstd_rand* engine) {
  std::vector<Value> inputs;
  for (Param* param : f->params()) {
    inputs.push_back(RandomValue(param->GetType(), engine));
  }
  return inputs;
}

Value F32ToTuple(float value) {
  uint32 x = absl::bit_cast<uint32>(value);
  bool sign = x >> 31;
  uint8 exp = x >> 23;
  uint32 fraction = x & Mask(23);
  return Value::Tuple({Value(Bits::FromBytes({sign}, /*bit_count=*/1)),
                       Value(Bits::FromBytes({exp}, /*bit_count=*/8)),
                       Value(UBits(fraction, /*bit_count=*/23))});
}

xabsl::StatusOr<float> TupleToF32(const Value& v) {
  XLS_ASSIGN_OR_RETURN(uint32 sign, v.element(0).bits().ToUint64());
  XLS_ASSIGN_OR_RETURN(uint32 exp, v.element(1).bits().ToUint64());
  XLS_ASSIGN_OR_RETURN(uint32 fraction, v.element(2).bits().ToUint64());
  // Validate the values were all appropriate.
  XLS_DCHECK_EQ(sign, sign & Mask(1));
  XLS_DCHECK_EQ(exp, exp & Mask(8));
  XLS_DCHECK_EQ(fraction, fraction & Mask(23));
  // Reconstruct the float.
  uint32 x = (sign << 31) | (exp << 23) | fraction;
  return absl::bit_cast<float>(x);
}

}  // namespace xls
