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

#ifndef NNACL_KERNEL_DEPTH_TO_SPACE_H_
#define NNACL_KERNEL_DEPTH_TO_SPACE_H_

#include "nnacl/op_base.h"
#include "nnacl/tensor_c.h"
#include "nnacl/kernel.h"

typedef struct DepthToSpaceArgs {
  int32_t in_stride_dim0_;
  int32_t in_stride_dim1_;
  int32_t in_stride_dim2_;
  int32_t out_stride_dim0_;
  int32_t out_stride_dim1_;
  int32_t out_stride_dim2_;
  uint8_t data_type_size_;
  int32_t block_size_;
} DepthToSpaceArgs;

typedef struct DepthToSpaceStruct {
  KernelBase base_;
  DepthToSpaceArgs args_;
} DepthToSpaceStruct;

KernelBase *CreateDepthToSpace(OpParameter *param, int data_type);

#endif  // NNACL_KERNEL_DEPTH_TO_SPACE_H_
