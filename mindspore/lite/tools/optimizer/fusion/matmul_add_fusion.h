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

#ifndef MINDSPORE_LITE_SRC_PASS_FUSION_MATMUL_ADD_FUSION_H_
#define MINDSPORE_LITE_SRC_PASS_FUSION_MATMUL_ADD_FUSION_H_

#include "backend/optimizer/common/optimizer.h"
#include "tools/converter/converter_context.h"
#include "backend/optimizer/common/pass.h"

namespace mindspore {
namespace opt {
class MatMulAddFusion : public Pass {
 public:
  MatMulAddFusion() : Pass("matmul_add_fusion") {}
  ~MatMulAddFusion() override = default;
  bool Run(const FuncGraphPtr &func_graph) override;
};
}  // namespace opt
}  // namespace mindspore
#endif  // MINDSPORE_LITE_SRC_PASS_FUSION_MATMUL_ADD_FUSION_H_
