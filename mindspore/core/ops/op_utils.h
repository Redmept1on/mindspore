/**
 * Copyright 2020-2023 Huawei Technologies Co., Ltd
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

#ifndef MINDSPORE_CORE_OPS_OP_UTILS_H
#define MINDSPORE_CORE_OPS_OP_UTILS_H
#include <algorithm>
#include <climits>
#include <memory>
#include <utility>
#include <set>
#include <string>
#include <vector>
#include "./op_name.h"
#include "abstract/ops/primitive_infer_map.h"
#include "mindapi/base/shape_vector.h"
#include "mindapi/base/shared_ptr.h"
#include "mindspore/core/ops/math_ops.h"

#ifndef MS_UNLIKELY
#ifdef _MSC_VER
#define MS_UNLIKELY(x) (x)
#define MS_LIKELY(x) (x)
#else
#define MS_LIKELY(x) __builtin_expect(!!(x), 1)
#define MS_UNLIKELY(x) __builtin_expect(!!(x), 0)
#endif
#endif
#define MS_CHECK_VALUE(cond, msg)        \
  do {                                   \
    if (MS_UNLIKELY(!(cond))) {          \
      MS_EXCEPTION(ValueError) << (msg); \
    }                                    \
  } while (0)

namespace mindspore::ops {
const std::set<TypePtr> common_valid_types = {kInt8,   kInt16,  kInt32,   kInt64,   kUInt8,   kUInt16,
                                              kUInt32, kUInt64, kFloat16, kFloat32, kFloat64, kBFloat16};
// ArrayValue functions as a std::vector that verifies unknown values. ArrayValue uses std::vector<T> to hold the
// contents of the Sequence or Tensor flattened elements and provides an interface to determine whether each element is
// ValueAny.
template <typename T>
class ArrayValue {
 public:
  ArrayValue(std::vector<T> &&data, std::set<size_t> &&unknown_value_indexes)
      : data_(std::move(data)), unknown_value_indexes_(std::move(unknown_value_indexes)) {}
  ~ArrayValue() = default;

  // Access the value of Array at the index position.
  // Note: The value at position index can not be unknown, otherwise throw an exception.
  const T &operator[](size_t index) const {
    if (index >= data_.size()) {
      MS_LOG(EXCEPTION) << "The index[" << index << "] is out of range, element size is: " << data_.size();
    }
    if (IsValueUnknown(index)) {
      MS_LOG(EXCEPTION) << "Try to get unknown value.";
    }
    return data_[index];
  }

  // Verify that the value at position index in ArrayValue is unknown.
  bool IsValueUnknown(size_t index) const { return unknown_value_indexes_.find(index) != unknown_value_indexes_.end(); }

  // Verify whether exist unknown value in ArrayValue.
  bool HasUnknownValue() const { return !unknown_value_indexes_.empty(); }

  // Convert the ArrayValue to std::vector, only work when there is no unknown value in ArrayValue.
  const std::vector<T> &ToVector() const {
    if (HasUnknownValue()) {
      MS_LOG(EXCEPTION) << "Can not convert vector, there is unknown value in ArrayValue.";
    }
    return data_;
  }

  // Convert the ArrayValue to a string which contains all element in ArrayValue.
  std::string ToString() const {
    std::ostringstream oss;
    size_t element_size = size();
    oss << "{ ";
    for (size_t i = 0; i < element_size; i++) {
      oss << std::to_string(data_[i]);
      if (i < element_size - 1) {
        oss << ", ";
      }
    }
    oss << " }";
    return oss.str();
  }

  // Get element number in ArrayValue.
  size_t size() const { return data_.size(); }

 private:
  // Use vector to hold the contents parsed from Sequence or Tensor Value.
  std::vector<T> data_;
  // Records the index whose value is unknown (ValueAny) in the data_ vector.
  std::set<size_t> unknown_value_indexes_;
};

// This interface is only used to get value for scalar data.
template <typename T>
std::optional<T> GetScalarValue(const ValuePtr &value);

// This interface is only used to convert values of type Sequence or Tensor to std::vector.
template <typename T>
std::optional<ArrayValue<T>> GetArrayValue(const ValuePtr &value);

const std::set<TypePtr> common_valid_types_with_bool = {
  kInt8, kInt16, kInt32, kInt64, kUInt8, kUInt16, kUInt32, kUInt64, kFloat16, kFloat32, kFloat64, kBool, kBFloat16};

const std::set<TypePtr> common_valid_types_with_complex = {kInt8,    kInt16,     kInt32,      kInt64,   kUInt8,
                                                           kUInt16,  kUInt32,    kUInt64,     kFloat16, kFloat32,
                                                           kFloat64, kComplex64, kComplex128, kBFloat16};

const std::set<TypePtr> common_valid_types_with_complex_and_bool = {
  kInt8,    kInt16,   kInt32,   kInt64,     kUInt8,      kUInt16, kUInt32,  kUInt64,
  kFloat16, kFloat32, kFloat64, kComplex64, kComplex128, kBool,   kBFloat16};

const std::set<TypePtr> common_integral_types = {kInt8, kInt16, kInt32, kInt64, kUInt8, kUInt16, kUInt32, kUInt64};
const std::set<TypePtr> common_float_types = {kFloat16, kFloat32, kFloat64, kBFloat16};
const std::set<TypePtr> all_types = {kBool,    kInt,     kInt8,    kInt16,     kInt32,      kInt64,
                                     kUInt,    kUInt8,   kUInt16,  kUInt32,    kUInt64,     kFloat,
                                     kFloat16, kFloat32, kFloat64, kComplex64, kComplex128, kBFloat16};
std::vector<int64_t> CalBroadCastShape(const std::vector<int64_t> &x_shape, const std::vector<int64_t> &y_shape,
                                       const std::string &op_name, const std::string &op_x_name = "input1",
                                       const std::string &op_y_name = "input2");
abstract::ShapePtr BroadCastInferShape(const std::string &op_name,
                                       const std::vector<abstract::AbstractBasePtr> &input_args);
BaseShapePtr EltwiseGradInferShape(const PrimitivePtr &primitive, const std::vector<AbstractBasePtr> &input_args);
TypePtr EltwiseGradInferType(const PrimitivePtr &primitive, const std::vector<AbstractBasePtr> &input_args);
void ReduceFuncCheckAxisInferImpl(const PrimitivePtr &prim, std::vector<int64_t> *axis, const size_t dim);
bool CheckAndGetAxisValue(const std::vector<abstract::AbstractBasePtr> &input_args, std::vector<int64_t> *axis_value,
                          int64_t *axis_shape_v, const PrimitivePtr &primitive);
ShapeVector ReduceFuncCalShapeAxisDyn(const ShapeVector &x_shape, bool keep_dims = false);
ShapeVector ReduceFuncCalShapeInferImpl(const PrimitivePtr &primitive, const ShapeVector &x_shape,
                                        const std::vector<int64_t> &axis, bool keep_dims_value = false);
abstract::ShapePtr ReduceBaseInferShape(const PrimitivePtr &primitive,
                                        const std::vector<abstract::AbstractBasePtr> &input_args,
                                        const std::string &prim_name);
TypePtr ReduceBaseInferType(const PrimitivePtr &prim, const std::vector<abstract::AbstractBasePtr> &input_args,
                            const std::set<TypePtr> &check_list);

template <typename T>
api::SharedPtr<T> GetOperator(const AnfNodePtr &node) {
  auto prim = GetValueNode<PrimitivePtr>(node);
  if (prim == nullptr) {
    return nullptr;
  }
  return api::MakeShared<T>(prim);
}

bool ObscureShapeEqual(const ShapeVector &lhs, const ShapeVector &rhs);

// Get the shape value from abstract input arg
// Ops like DynamicBroadcastTo or Reshape can directly get the shape value
// from input which represents shape by invoking this function
// Do not support input with type of AbstractTuple of AbstractTensor
ShapeVector GetShapeValue(const PrimitivePtr &primitive, const AbstractBasePtr &input_arg);

inline ShapeVector ConvertBaseShapeToTensorShape(const BaseShapePtr &base) {
  auto shape_ptr = base->cast<abstract::ShapePtr>();
  MS_EXCEPTION_IF_NULL(shape_ptr);
  return shape_ptr->shape();
}

inline ShapeVector GetShapeFromTensor(const AbstractBasePtr &abs) {
  auto base_shape = abs->BuildShape();
  return ConvertBaseShapeToTensorShape(base_shape);
}

// Infer shape value of make-shape op that only transform shapes, e.g. Concat, Stack, StridedSlice
// Do not support op with multiple outputs for now
ValuePtr InferMakeShapeTensorValue(const PrimitivePtr &prim, const AbstractBasePtrList &args);

// Infer shape value of compute-shape op that could change the dim value, e.g. Mul, Add, Sub
// Do not support op with multiple outputs for now
ValuePtr InferComputeShapeTensorValue(const PrimitivePtr &prim, const AbstractBasePtrList &args);

void CheckSparseShape(ShapeVector sparse_shp, ShapeVector dense_shp);

void CheckSparseShape(const size_t shape_size, const size_t expected_dim, const std::string &arg_name);

void CheckSparseIndicesDtype(const TypePtr data_type, const std::string &arg_name);

void CheckSparseIndicesDtypeInt32(const TypePtr data_type, const std::string &arg_name);

inline void CheckInputShapeEmpty(const std::string &prim_name, const std::vector<AbstractBasePtr> &input_args) {
  for (size_t i = 0; i < input_args.size(); ++i) {
    MS_EXCEPTION_IF_NULL(input_args[i]->BuildShape());
    if (input_args[i]->BuildShape()->IsDimZero()) {
      MS_LOG(EXCEPTION) << "For '" << prim_name << "', input " << i << "'s shape should not be empty!";
    }
  }
}

ShapeVector ConvertToShapeVector(const abstract::AbstractTuplePtr &shape);

template <typename T>
std::shared_ptr<T> InferSparseAttr(const PrimitivePtr &primitive, const AbstractBasePtrList &args_abs_list);

template <typename T>
AbstractBasePtr TensorToSequenceInfer(const PrimitivePtr &primitive, const std::vector<AbstractBasePtr> &input_args);

template <typename T>
AbstractBasePtr InferSequenceSetItem(const PrimitivePtr &primitive, const AbstractBasePtrList &args_abs_list);

template <typename T>
T GetScalarCastValue(const std::string &op_name, const ValuePtr &elem);

TypePtr HighPriorityType(const TypePtr &x_type, const TypePtr &y_type, const std::string &op_name);

bool IsValueKnown(const ValuePtr &value);

MS_CORE_API size_t GetInputIndexByName(const std::string &op_name, const std::string &input_name);

constexpr auto kCSRAvgRows = "csr_avg_rows";
constexpr auto kIsCSR = "is_csr";
constexpr auto kCSRDenseShape = "dense_shape";
constexpr auto kCSRAxis = "axis";
}  // namespace mindspore::ops
#endif  // MINDSPORE_CORE_OPS_OP_UTILS_H
