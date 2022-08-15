/**
 * Copyright 2019-2022 Huawei Technologies Co., Ltd
 *
 * Licensed under the Apache License, Version 2.0 (the "License");
 * you may not use this file except in compliance with the License.
 * You may obtain a copy of the License at
 *
 * http://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an "AS IS" BASIS,
 * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 * See the License for the specific language governing permissions and
 * limitations under the License.
 */

#include "ir/dtype/container.h"
#include <cstdlib>
#include <algorithm>
#include "utils/log_adapter.h"
#include "utils/ms_utils.h"

namespace mindspore {
static std::string DumpTypeVector(const std::vector<TypePtr> &elements, bool is_dumptext) {
  std::ostringstream oss;
  bool begin = true;
  size_t cnt = 0;
  // write 'Tuple[Bool, Bool, Bool, Int, Float, Float]' as 'Tuple[Bool...3, Int, Float...2]'
  for (size_t i = 0; i < elements.size(); ++i) {
    TypePtr elem = elements[i];
    cnt += 1;
    bool print = false;
    if (i + 1 < elements.size()) {
      TypePtr next = elements[i + 1];
      if (*elem != *next) {
        print = true;
      }
    } else {
      // encounter last element
      print = true;
    }
    if (!print) {
      continue;
    }
    if (!begin) {
      oss << ",";
    } else {
      begin = false;
    }
    oss << (is_dumptext ? elem->DumpText() : elem->ToString());
    if (cnt > 1) {
      oss << "*" << cnt;
    }
    cnt = 0;
  }
  return oss.str();
}

TypePtr List::DeepCopy() const {
  if (IsGeneric()) {
    return std::make_shared<List>();
  } else {
    TypePtrList elements;
    (void)std::transform(elements_.begin(), elements_.end(), std::back_inserter(elements),
                         [](const TypePtr &ele) { return ele->DeepCopy(); });
    auto copy = std::make_shared<List>(elements);
    return copy;
  }
}

const TypePtr List::operator[](std::size_t dim) const {
  if (dim >= size()) {
    MS_LOG(EXCEPTION) << "Index " << dim << " is out range of the List size " << size() << ".";
  }
  return elements_[dim];
}

bool List::operator==(const Type &other) const {
  if (!IsSameObjectType(*this, other)) {
    return false;
  }
  const List &other_list = static_cast<const List &>(other);
  if (elements_.size() != other_list.elements_.size()) {
    return false;
  }
  for (size_t i = 0; i < elements_.size(); ++i) {
    if (!common::IsEqual(elements_[i], other_list.elements_[i])) {
      return false;
    }
  }
  return true;
}

size_t List::hash() const {
  size_t hash_value = hash_combine(static_cast<size_t>(kMetaTypeObject), static_cast<size_t>(object_type()));
  hash_value = hash_combine(hash_value, elements_.size());
  for (auto &e : elements_) {
    hash_value = hash_combine(hash_value, (e == nullptr ? 0 : e->hash()));
  }
  return hash_value;
}

std::string List::DumpContent(bool is_dumptext) const {
  std::ostringstream buffer;
  if (IsGeneric()) {
    buffer << "List";
  } else {
    buffer << "List[";
    buffer << DumpTypeVector(elements_, is_dumptext);
    buffer << "]";
  }
  return buffer.str();
}

TypePtr Tuple::DeepCopy() const {
  if (IsGeneric()) {
    return std::make_shared<Tuple>();
  } else {
    TypePtrList elements;
    (void)std::transform(elements_.begin(), elements_.end(), std::back_inserter(elements),
                         [](const TypePtr &ele) { return ele->DeepCopy(); });
    auto copy = std::make_shared<Tuple>(elements);
    return copy;
  }
}

bool Tuple::operator==(const Type &other) const {
  if (!IsSameObjectType(*this, other)) {
    return false;
  }
  auto other_tuple = static_cast<const Tuple &>(other);
  if (elements_.size() != other_tuple.elements_.size()) {
    return false;
  }
  for (size_t i = 0; i < elements_.size(); ++i) {
    if (!common::IsEqual(elements_[i], other_tuple.elements_[i])) {
      return false;
    }
  }
  return true;
}

size_t Tuple::hash() const {
  size_t hash_value = hash_combine(static_cast<size_t>(kMetaTypeObject), static_cast<size_t>(object_type()));
  hash_value = hash_combine(hash_value, elements_.size());
  for (auto &e : elements_) {
    hash_value = hash_combine(hash_value, (e == nullptr ? 0 : e->hash()));
  }
  return hash_value;
}

const TypePtr Tuple::operator[](std::size_t dim) const {
  if (dim >= size()) {
    MS_LOG(EXCEPTION) << "Index " << dim << " is out range of the Tuple size " << size() << ".";
  }
  return elements_[dim];
}

std::string Tuple::DumpContent(bool is_dumptext) const {
  std::ostringstream buffer;
  if (IsGeneric()) {
    buffer << "Tuple";
  } else {
    buffer << "Tuple[";
    buffer << DumpTypeVector(elements_, is_dumptext);
    buffer << "]";
  }
  return buffer.str();
}

TypePtr Dictionary::DeepCopy() const {
  if (IsGeneric()) {
    return std::make_shared<Dictionary>();
  } else {
    std::vector<std::pair<std::string, TypePtr>> kv;
    (void)std::transform(
      key_values_.begin(), key_values_.end(), std::back_inserter(kv),
      [](const std::pair<std::string, TypePtr> &item) { return std::make_pair(item.first, item.second->DeepCopy()); });
    return std::make_shared<Dictionary>(kv);
  }
}

std::string DumpKeyVector(std::vector<std::string> keys) {
  std::ostringstream buffer;
  for (auto key : keys) {
    buffer << key << ",";
  }
  return buffer.str();
}

std::string Dictionary::DumpContent(bool) const {
  std::ostringstream buffer;
  std::vector<std::string> keys;
  std::vector<TypePtr> values;
  for (const auto &kv : key_values_) {
    keys.push_back(kv.first);
    values.push_back(kv.second);
  }
  if (IsGeneric()) {
    buffer << "Dictionary";
  } else {
    buffer << "Dictionary[";
    buffer << "[" << DumpKeyVector(keys) << "],";
    buffer << "[" << DumpTypeVector(values, false) << "]";
    buffer << "]";
  }
  return buffer.str();
}

bool Dictionary::operator==(const mindspore::Type &other) const {
  if (!IsSameObjectType(*this, other)) {
    return false;
  }

  const auto &other_dict = static_cast<const Dictionary &>(other);
  const auto size = key_values_.size();
  if (size != other_dict.key_values_.size()) {
    return false;
  }
  for (size_t i = 0; i < size; ++i) {
    const auto &a = key_values_[i];
    const auto &b = other_dict.key_values_[i];
    if (a.first != b.first || !common::IsEqual(a.second, b.second)) {
      return false;
    }
  }
  return true;
}

size_t Dictionary::hash() const {
  size_t hash_value = hash_combine(static_cast<size_t>(kMetaTypeObject), static_cast<size_t>(object_type()));
  hash_value = hash_combine(hash_value, key_values_.size());
  for (auto &kv : key_values_) {
    hash_value = hash_combine(hash_value, std::hash<std::string>{}(kv.first));
    hash_value = hash_combine(hash_value, (kv.second == nullptr ? 0 : kv.second->hash()));
  }
  return hash_value;
}
}  // namespace mindspore
