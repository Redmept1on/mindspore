/**
 * Copyright 2023 Huawei Technologies Co., Ltd
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

#ifndef MINDSPORE_CORE_OPS_OPS_FUNC_IMPL_SCALAR_LOG_H_
#define MINDSPORE_CORE_OPS_OPS_FUNC_IMPL_SCALAR_LOG_H_

#include <vector>
#include "ops/ops_func_impl/scalar_arithmetic_unary.h"

namespace mindspore {
namespace ops {
class MIND_API ScalarLogFuncImpl : public ScalarArithmeticUnaryFuncImpl {
 public:
  inline TypePtr InferType(const PrimitivePtr &primitive,
                           const std::vector<AbstractBasePtr> &input_args) const override {
    return kFloat32;
  }
};

class ScalarLogFrontendFuncImpl : public ScalarArithmeticUnaryFrontendFuncImpl {};

REGISTER_PRIMITIVE_FUNCTION_FRONTEND_FUNC_IMPL("ScalarLog", ScalarLogFrontendFuncImpl);
}  // namespace ops
}  // namespace mindspore

#endif  // MINDSPORE_CORE_OPS_OPS_FUNC_IMPL_SCALAR_LOG_H_
