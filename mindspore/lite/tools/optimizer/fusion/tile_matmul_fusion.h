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

#ifndef MINDSPORE_LITE_TOOLS_OPTIMIZER_FUSION_TILE_MATMUL_FUSION_H_
#define MINDSPORE_LITE_TOOLS_OPTIMIZER_FUSION_TILE_MATMUL_FUSION_H_

#include <string>
#include "tools/optimizer/common/multiple_pattern_process_pass.h"
#include "utils/check_convert_utils.h"

namespace mindspore {
namespace opt {
class TileMatMulFusion : public Pass {
 public:
  TileMatMulFusion() : Pass("TileMatMulFusion") {}
  ~TileMatMulFusion() override = default;
  bool Run(const FuncGraphPtr &func_graph) override;

 private:
  bool CheckCanFuse(const FuncGraphPtr &func_graph, const AnfNodePtr &node) const;
};
}  // namespace opt
}  // namespace mindspore
#endif  // MINDSPORE_LITE_TOOLS_OPTIMIZER_FUSION_TILE_MATMUL_FUSION_H_
