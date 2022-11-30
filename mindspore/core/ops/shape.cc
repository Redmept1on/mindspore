/**
 * Copyright 2020-2021 Huawei Technologies Co., Ltd
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

#include "ops/shape.h"
#include <string>
#include <algorithm>
#include <memory>
#include <set>
#include <vector>
#include "ops/op_utils.h"
#include "utils/check_convert_utils.h"
#include "abstract/ops/primitive_infer_map.h"
#include "mindapi/src/helper.h"

namespace mindspore {
namespace ops {
MIND_API_OPERATOR_IMPL(Shape, BaseOperator);
AbstractBasePtr ShapeInfer(const abstract::AnalysisEnginePtr &, const PrimitivePtr &primitive,
                           const std::vector<AbstractBasePtr> &input_args) {
  // Only called when the input of shape is dynamic shape/rank tensor.
  // infer shape
  MS_EXCEPTION_IF_NULL(primitive);
  auto op_name = primitive->name();
  (void)CheckAndConvertUtils::CheckInteger("shape infer", int64_t(input_args.size()), kEqual, 1, op_name);
  MS_EXCEPTION_IF_NULL(input_args[0]);
  auto shape_map = CheckAndConvertUtils::ConvertShapePtrToShapeMap(input_args[0]->BuildShape());
  auto in_shape = shape_map[kShape];
  // infer type
  std::set<TypePtr> valid_params_types = {kTensorType};
  (void)CheckAndConvertUtils::CheckSubClass("shape type", input_args[0]->BuildType(), valid_params_types, op_name);
  AbstractBasePtrList abs_list;
  // Input of shape is not dynamic rank Tensor.
  (void)std::transform(in_shape.begin(), in_shape.end(), std::back_inserter(abs_list),
                       [](int64_t item) -> std::shared_ptr<abstract::AbstractScalar> {
                         auto ret = std::make_shared<abstract::AbstractScalar>(item);
                         if (item == abstract::Shape::kShapeRankAny || item == abstract::Shape::kShapeDimAny) {
                           ret->set_value(kAnyValue);
                         }
                         return ret;
                       });
  auto abs = std::make_shared<abstract::AbstractTuple>(abs_list);
  if (IsDynamicRank(in_shape)) {
    abs->CheckAndConvertToDynamicLenSequence();
  }
  return abs;
}

ValuePtr ShapeInferValue(const PrimitivePtr &primitive, const std::vector<AbstractBasePtr> &input_args) {
  MS_EXCEPTION_IF_NULL(primitive);
  auto op_name = primitive->name();
  (void)CheckAndConvertUtils::CheckInteger("shape infer", int64_t(input_args.size()), kEqual, 1, op_name);
  MS_EXCEPTION_IF_NULL(input_args[0]);
  std::set<TypePtr> valid_params_types = {kTensorType};
  (void)CheckAndConvertUtils::CheckSubClass("shape type", input_args[0]->BuildType(), valid_params_types, op_name);
  auto shape_map = CheckAndConvertUtils::ConvertShapePtrToShapeMap(input_args[0]->BuildShape());
  auto inshape = shape_map[kShape];
  if (std::any_of(inshape.begin(), inshape.end(), [](ShapeValueDType shape) {
        return shape == abstract::Shape::kShapeRankAny || shape == abstract::Shape::kShapeDimAny;
      })) {
    // If the input of shape is dynamic shape/rank tensor, value can not be directly built.
    // Run infer of shape.
    return nullptr;
  }
  return MakeValue(inshape);
}
REGISTER_PRIMITIVE_EVAL_IMPL(Shape, prim::kPrimShape, ShapeInfer, ShapeInferValue, true);
}  // namespace ops
}  // namespace mindspore
