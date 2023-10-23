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

#include "plugin/device/ascend/kernel/pyboost/baddbmm_ascend.h"
#include <algorithm>
#include <functional>
#include <memory>
#include "ir/tensor.h"
#include "runtime/device/device_address_utils.h"
#include "runtime/hardware/device_context_manager.h"
#include "transform/acl_ir/op_api_exec.h"

namespace mindspore {
namespace kernel {
namespace pyboost {
bool BaddbmmAscend::Launch(const tensor::TensorPtr &input, const tensor::TensorPtr &batch1,
                           const tensor::TensorPtr &batch2, const ScalarPtr &beta, const ScalarPtr &alpha,
                           const tensor::TensorPtr &output) {
  auto device_context = device::DeviceContextManager::GetInstance().GetOrCreateDeviceContext(
    {kAscendDevice, MsContext::GetInstance()->get_param<uint32_t>(MS_CTX_DEVICE_ID)});
  MS_EXCEPTION_IF_NULL(device_context);
  device_context->Initialize();

  MS_EXCEPTION_IF_NULL(device_context->device_res_manager_);
  device_context->device_res_manager_->BindDeviceToCurrentThread(false);

  runtime::DeviceAddressUtils::CreateInputTensorAddress(device_context, input, "input");
  runtime::DeviceAddressUtils::CreateInputTensorAddress(device_context, batch1, "batch1");
  runtime::DeviceAddressUtils::CreateInputTensorAddress(device_context, batch2, "batch2");
  runtime::DeviceAddressUtils::CreateOutputTensorAddress(device_context, output, "output");

  // 910A not support 0
  int8_t cube_math_type = 0;
  auto [workspace_size, executor, after_launch_func] =
    GEN_EXECUTOR(aclnnBaddbmm, input, batch1, batch2, beta, alpha, output, cube_math_type);

  auto stream_ptr = device_context->device_res_manager_->GetStream(kDefaultStreamIndex);
  if (workspace_size == 0) {
    RUN_OP_API(aclnnBaddbmm, stream_ptr, nullptr, 0, executor, after_launch_func);
  } else {
    auto workspace_device_address = runtime::DeviceAddressUtils::CreateWorkspaceAddress(device_context, workspace_size);
    RUN_OP_API(aclnnBaddbmm, stream_ptr, workspace_device_address->GetMutablePtr(), workspace_size, executor,
               after_launch_func);
  }

  return true;
}

tensor::TensorPtr BaddbmmAscend::Call(const tensor::TensorPtr &input, const tensor::TensorPtr &batch1,
                                      const tensor::TensorPtr &batch2, const ScalarPtr &beta, const ScalarPtr &alpha) {
  MS_LOG(DEBUG) << "Call start";
  Infer(primitive_, input, batch1, batch2, beta, alpha);
  MS_LOG(DEBUG) << "Infer end";

  if (outputs_.size() != 1) {
    MS_LOG(EXCEPTION) << "Baddbmm output size should be 1, but got " << outputs_.size();
  }

  Launch(input, batch1, batch2, beta, alpha, outputs_[0]);
  MS_LOG(DEBUG) << "Launch end";
  return outputs_[0];
}
}  // namespace pyboost
}  // namespace kernel
}  // namespace mindspore
