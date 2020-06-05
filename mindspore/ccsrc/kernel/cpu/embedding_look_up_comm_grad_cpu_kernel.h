/**
 * Copyright 2020 Huawei Technologies Co., Ltd
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
#ifndef MINDSPORE_CCSRC_KERNEL_CPU_EMBEDDING_LOOK_UP_COMM_GRAD_CPU_KERNEL_H_
#define MINDSPORE_CCSRC_KERNEL_CPU_EMBEDDING_LOOK_UP_COMM_GRAD_CPU_KERNEL_H_
#include <vector>
#include <memory>
#include "kernel/cpu/cpu_kernel.h"
#include "kernel/cpu/cpu_kernel_factory.h"

namespace mindspore {
namespace kernel {
class EmbeddingLookUpCommGradCPUKernel : public CPUKernel {
 public:
  EmbeddingLookUpCommGradCPUKernel() : split_num_(1) {}
  ~EmbeddingLookUpCommGradCPUKernel() override{};

  void InitKernel(const CNodePtr &kernel_node) override;

  bool Launch(const std::vector<AddressPtr> &inputs, const std::vector<AddressPtr> &workspace,
              const std::vector<AddressPtr> &outputs) override;

 private:
  void CheckParam(const CNodePtr &kernel_node);
  int split_num_;
};

MS_REG_CPU_KERNEL(EmbeddingLookupCommGrad,
                  KernelAttr().AddInputAttr(kNumberTypeFloat32).AddOutputAttr(kNumberTypeFloat32),
                  EmbeddingLookUpCommGradCPUKernel);
}  // namespace kernel
}  // namespace mindspore

#endif  // MINDSPORE_CCSRC_KERNEL_CPU_EMBEDDING_LOOK_UP_COMM_GRAD_CPU_KERNEL_H_
