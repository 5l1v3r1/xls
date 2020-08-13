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

#include "xls/ir/type.h"

#include "absl/strings/str_cat.h"
#include "absl/strings/str_format.h"
#include "absl/strings/str_join.h"

namespace xls {

std::string TypeKindToString(TypeKind type_kind) {
  switch (type_kind) {
    case TypeKind::kTuple:
      return "tuple";
    case TypeKind::kBits:
      return "bits";
    case TypeKind::kArray:
      return "array";
    case TypeKind::kToken:
      return "token";
  }
  return absl::StrFormat("<invalid TypeKind %d>", static_cast<int>(type_kind));
}

std::ostream& operator<<(std::ostream& os, TypeKind type_kind) {
  os << TypeKindToString(type_kind);
  return os;
}

xabsl::StatusOr<BitsType*> Type::AsBits() {
  if (IsBits()) {
    return AsBitsOrDie();
  }
  return absl::InvalidArgumentError("Type is not 'bits': " + ToString());
}

xabsl::StatusOr<ArrayType*> Type::AsArray() {
  if (IsArray()) {
    return AsArrayOrDie();
  }
  return absl::InvalidArgumentError("Type is not an array: " + ToString());
}

TypeProto BitsType::ToProto() const {
  TypeProto proto;
  proto.set_type_enum(TypeProto::BITS);
  proto.set_bit_count(bit_count());
  return proto;
}

bool BitsType::IsEqualTo(const Type* other) const {
  if (this == other) {
    return true;
  }
  return other->IsBits() && bit_count() == other->AsBitsOrDie()->bit_count();
}

TypeProto TupleType::ToProto() const {
  TypeProto proto;
  proto.set_type_enum(TypeProto::TUPLE);
  for (Type* element : element_types()) {
    *proto.add_tuple_elements() = element->ToProto();
  }
  return proto;
}

bool TupleType::IsEqualTo(const Type* other) const {
  if (this == other) {
    return true;
  }
  if (!other->IsTuple()) {
    return false;
  }
  const TupleType* other_tuple = other->AsTupleOrDie();
  if (size() != other_tuple->size()) {
    return false;
  }
  for (int64 i = 0; i < size(); ++i) {
    if (!element_type(i)->IsEqualTo(other_tuple->element_type(i))) {
      return false;
    }
  }
  return true;
}

TypeProto ArrayType::ToProto() const {
  TypeProto proto;
  proto.set_type_enum(TypeProto::ARRAY);
  proto.set_array_size(size());
  *proto.mutable_array_element() = element_type()->ToProto();
  return proto;
}

bool ArrayType::IsEqualTo(const Type* other) const {
  if (this == other) {
    return true;
  }
  if (!other->IsArray()) {
    return false;
  }
  const ArrayType* other_array = other->AsArrayOrDie();
  return size() == other_array->size() &&
         element_type()->IsEqualTo(other_array->element_type());
}

TypeProto TokenType::ToProto() const {
  TypeProto proto;
  proto.set_type_enum(TypeProto::TOKEN);
  return proto;
}

bool TokenType::IsEqualTo(const Type* other) const {
  if (this == other) {
    return true;
  }
  return other->IsToken();
}

std::ostream& operator<<(std::ostream& os, const Type& type) {
  os << type.ToString();
  return os;
}

std::string TupleType::ToString() const {
  std::vector<std::string> pieces;
  for (Type* member : members_) {
    pieces.push_back(member->ToString());
  }
  return absl::StrCat("(", absl::StrJoin(pieces, ", "), ")");
}

BitsType::BitsType(int64 bit_count)
    : Type(TypeKind::kBits), bit_count_(bit_count) {
  XLS_CHECK_GE(bit_count_, 0);
}

std::string BitsType::ToString() const {
  return absl::StrFormat("bits[%d]", bit_count());
}

std::string ArrayType::ToString() const {
  return absl::StrFormat("%s[%d]", element_type()->ToString(), size());
}

std::string TokenType::ToString() const { return absl::StrFormat("token"); }

FunctionTypeProto FunctionType::ToProto() const {
  FunctionTypeProto proto;
  for (Type* parameter : parameters()) {
    *proto.add_parameters() = parameter->ToProto();
  }
  *proto.mutable_return_type() = return_type()->ToProto();
  return proto;
}

bool FunctionType::IsEqualTo(const FunctionType* other) const {
  if (this == other) {
    return true;
  }
  if (!return_type()->IsEqualTo(other->return_type())) {
    return false;
  }
  if (parameter_count() != other->parameter_count()) {
    return false;
  }
  for (int64 i = 0; i < parameter_count(); ++i) {
    if (!parameter_type(i)->IsEqualTo(other->parameter_type(i))) {
      return false;
    }
  }
  return true;
}

std::string FunctionType::ToString() const {
  std::vector<std::string> pieces;
  for (Type* parameter : parameters()) {
    pieces.push_back(parameter->ToString());
  }
  return absl::StrCat("(", absl::StrJoin(pieces, ", "), ") -> ",
                      return_type()->ToString());
}

std::ostream& operator<<(std::ostream& os, const Type* type) {
  os << (type == nullptr ? std::string("<nullptr Type*>") : type->ToString());
  return os;
}

}  // namespace xls
