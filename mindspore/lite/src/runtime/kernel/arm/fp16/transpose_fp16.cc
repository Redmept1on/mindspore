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

#include "src/runtime/kernel/arm/fp16/transpose_fp16.h"
#include <vector>
#include "nnacl/fp16/transpose_fp16.h"
#include "schema/model_generated.h"
#include "src/kernel_registry.h"
#include "include/errorcode.h"
#include "src/runtime/runtime_api.h"
#include "nnacl/fp16/cast_fp16.h"

using mindspore::lite::KernelRegistrar;
using mindspore::lite::RET_ERROR;
using mindspore::lite::RET_OK;
using mindspore::lite::RET_OP_EXECUTE_FAILURE;
using mindspore::schema::PrimitiveType_Transpose;

namespace mindspore::kernel {
int TransposeFp16CPUKernel::Init() {
  if (!InferShapeDone()) {
    return RET_OK;
  }
  return TransposeCPUKernel::ReSize();
}

int TransposeFp16CPUKernel::Run() {
  MS_ASSERT(in_tensors_.size() == 1 || in_tensors_.size() == 2);
  TransposeParameter *param = reinterpret_cast<TransposeParameter *>(this->op_parameter_);
  param->data_size_ = in_tensors_[0]->Size();
  MS_ASSERT(out_tensors_.size() == 1);
  auto &in_tensor = in_tensors_.front();
  auto &out_tensor = out_tensors_.front();
  if (in_tensor == nullptr || out_tensor == nullptr) {
    MS_LOG(ERROR) << "null pointer referencing.";
    return RET_ERROR;
  }
  in_data_fp16_ = reinterpret_cast<float16_t *>(in_tensor->MutableData());
  out_data_fp16_ = reinterpret_cast<float16_t *>(out_tensor->MutableData());
  MS_ASSERT(in_data_fp16_);
  MS_ASSERT(out_data_fp16_);

  if (in_tensor->shape().size() != static_cast<size_t>(param->num_axes_)) {
    memcpy(out_data_fp16_, in_data_fp16_, in_tensor->ElementsNum() * sizeof(float16_t));
    return RET_OK;
  }

  MS_ASSERT(out_shape_);
  auto ret = Fp16DoTranspose(in_data_fp16_, out_data_fp16_, out_shape_, param);
  if (ret != RET_OK) {
    MS_LOG(ERROR) << "Transpose run failed";
    return RET_ERROR;
  }

  return ret;
}

REG_KERNEL(kCPU, kNumberTypeFloat16, PrimitiveType_Transpose, LiteKernelCreator<TransposeFp16CPUKernel>)
}  // namespace mindspore::kernel
