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

#include "ops/view/transpose_strides_calc.h"
#include <vector>
#include <memory>

namespace mindspore::ops {
TensorStorageInfoPtrList TransposeCalc(const PrimitivePtr &prim, const std::vector<ValuePtr> &inputs) {
  if (inputs.size() != 2) {
    MS_LOG(EXCEPTION) << "inputs is invalid, size:" << inputs.size();
  }

  if (inputs[0] == nullptr || inputs[1] == nullptr) {
    MS_LOG(EXCEPTION) << "input is null";
  }

  if (!inputs[0]->isa<tensor::Tensor>()) {
    MS_LOG(EXCEPTION) << "inputs is invalid, size:" << inputs.size();
  }

  if (!inputs[1]->isa<ValueSequence>()) {
    MS_LOG(EXCEPTION) << "inputs is invalid, size:" << inputs.size();
  }

  auto tensor = inputs[0]->cast<tensor::TensorPtr>();
  MS_EXCEPTION_IF_NULL(tensor);

  auto old_tensor_info = GetOldTensorInfo(tensor);
  auto old_shape = old_tensor_info->old_shape;
  auto old_strides = old_tensor_info->old_strides;
  auto old_storage_offset = old_tensor_info->old_offset;

  auto dims = GetValue<std::vector<int64_t>>(inputs[1]);
  const auto ndim = old_shape.size();

  ShapeVector new_shape(ndim);
  std::vector<int64_t> new_strides(ndim);
  std::vector<bool> seen_dims(ndim, false);

  for (size_t i = 0; i < ndim; i++) {
    const auto wrap_dim = DynamicDimWrap(dims[i], ndim);
    if (seen_dims[wrap_dim]) {
      MS_LOG(EXCEPTION) << "duplicate dims";
    }
    seen_dims[wrap_dim] = true;
    new_shape[i] = old_shape[wrap_dim];
    new_strides[i] = old_strides[wrap_dim];
  }

  bool is_contiguouts = IsContiguous(new_shape, new_strides);
  auto new_storage_info =
    std::make_shared<TensorStorageInfo>(new_shape, new_strides, old_storage_offset, old_tensor_info->ori_shape,
                                        old_tensor_info->ori_strides, is_contiguouts);
  return {new_storage_info};
}

REG_VIEW_STRIDES_CALC_FUN(Transpose, TransposeCalc);
}  // namespace mindspore::ops
