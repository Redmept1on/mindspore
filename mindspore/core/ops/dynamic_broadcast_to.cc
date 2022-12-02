/**
 * Copyright 2021 Huawei Technologies Co., Ltd
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

#include "ops/dynamic_broadcast_to.h"

#include <set>
#include "utils/check_convert_utils.h"
#include "ops/op_utils.h"

namespace mindspore {
namespace ops {
namespace {
inline void CheckShapeValid(const ShapeVector &x_shape, const ShapeVector &output_shape) {
  if (IsDynamic(x_shape) || IsDynamic(output_shape)) {
    return;
  }

  if (x_shape.size() > output_shape.size()) {
    MS_EXCEPTION(ValueError) << "Not support shapes for broadcast, x_shape: " << x_shape
                             << ", target shape: " << output_shape;
  }
  auto outer_dim_offset = output_shape.size() - x_shape.size();

  for (size_t i = 0; i < x_shape.size(); ++i) {
    if (output_shape[i + outer_dim_offset] != x_shape[i] && x_shape[i] != 1) {
      MS_EXCEPTION(ValueError) << "Not support shapes for broadcast, x_shape: " << x_shape
                               << ", target shape: " << output_shape;
    }
  }
}
abstract::ShapePtr DynamicBroadcastToInferShape(const PrimitivePtr &primitive,
                                                const std::vector<AbstractBasePtr> &input_args) {
  MS_EXCEPTION_IF_NULL(primitive);
  auto prim_name = primitive->name();
  const int64_t input_num = 2;
  (void)CheckAndConvertUtils::CheckInteger("input number", SizeToLong(input_args.size()), kEqual, input_num, prim_name);
  auto x_shape = CheckAndConvertUtils::ConvertShapePtrToShapeMap(input_args[0]->BuildShape())[kShape];
  auto input_y = input_args[1];
  MS_EXCEPTION_IF_NULL(input_y);
  auto output_shape = GetShapeValue(primitive, input_y);
  CheckShapeValid(x_shape, output_shape);
  return std::make_shared<abstract::Shape>(output_shape);
}

TypePtr DynamicBroadcastToInferType(const PrimitivePtr &prim, const std::vector<AbstractBasePtr> &input_args) {
  for (const auto &item : input_args) {
    MS_EXCEPTION_IF_NULL(item);
  }
  auto x_dtype = input_args[0]->BuildType()->cast<TensorTypePtr>();
  std::set<TypePtr> template_types = {kTensorType};
  (void)CheckAndConvertUtils::CheckSubClass("x_dtype", x_dtype, template_types, prim->name());
  return x_dtype->element();
}
}  // namespace

MIND_API_OPERATOR_IMPL(DynamicBroadcastTo, BaseOperator);
AbstractBasePtr DynamicBroadcastToInfer(const abstract::AnalysisEnginePtr &, const PrimitivePtr &primitive,
                                        const std::vector<AbstractBasePtr> &input_args) {
  return abstract::MakeAbstract(DynamicBroadcastToInferShape(primitive, input_args),
                                DynamicBroadcastToInferType(primitive, input_args));
}
REGISTER_PRIMITIVE_EVAL_IMPL(DynamicBroadcastTo, prim::kPrimDynamicBroadcastTo, DynamicBroadcastToInfer, nullptr, true);
}  // namespace ops
}  // namespace mindspore
