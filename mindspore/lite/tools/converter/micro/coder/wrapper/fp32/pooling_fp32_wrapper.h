/**
 * Copyright 2022 Huawei Technologies Co., Ltd
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

#ifndef MINDSPORE_LITE_MICRO_CODER_OPERATOR_LIBRARY_WRAPPER_FP32_POOLING_FP32_WRAPPER_H_
#define MINDSPORE_LITE_MICRO_CODER_OPERATOR_LIBRARY_WRAPPER_FP32_POOLING_FP32_WRAPPER_H_
#include <string.h>
#include "nnacl/fp32/pooling_fp32.h"

typedef struct {
  const float *input_;
  float *output_;
  const PoolingParameter *pooling_param_;
  const PoolingComputeParam *pooling_args_;
} PoolingFp32Args;

#ifdef __cplusplus
extern "C" {
#endif

int DoMaxPooling(void *cdata, int task_id, float lhs_scale, float rhs_scale);

#ifdef __cplusplus
}
#endif
#endif  // MINDSPORE_LITE_MICRO_CODER_OPERATOR_LIBRARY_WRAPPER_FP32_POOLING_FP32_WRAPPER_H_
