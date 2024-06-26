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

#ifndef NNACL_FP16_GRAD_PACK_FP16_EXT_H_
#define NNACL_FP16_GRAD_PACK_FP16_EXT_H_

#include <stddef.h>
#include "nnacl/conv_parameter.h"

#ifdef __cplusplus
extern "C" {
#endif

void RollingIm2ColPackUnitFp16(const float16_t *input_data, const ConvParameter *conv_param, float16_t *packed_input,
                               int real_cal_num, int block_index);
void RollingIm2ColPackDwUnitFp16(const float16_t *input_data, const ConvParameter *conv_param, float16_t *packed_input,
                                 int real_cal_num, int block_index);
void RollingCol2ImPackUnitFp16(const float16_t *data_col, float16_t *data_im, const ConvParameter *conv_param,
                               int real_cal_num, int block_index);
#ifdef __cplusplus
}
#endif

#endif  // NNACL_FP16_GRAD_PACK_FP16_EXT_H_
