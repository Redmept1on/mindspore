/**
 * Copyright 2021-2022 Huawei Technologies Co., Ltd
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
#include "nnacl/fp32/sub_fp32.h"
#include "nnacl/intrinsics/ms_simd_instructions.h"
#ifdef ENABLE_AVX512
#include "nnacl/avx512/sub_fp32_avx512.h"
#endif

#ifdef ENABLE_AVX
#include "nnacl/avx/sub_fp32_avx.h"
#endif

#ifdef ENABLE_SSE
#include "nnacl/sse/sub_fp32_sse.h"
#endif

#ifdef ENABLE_ARM
#include "nnacl/neon/sub_fp32_neon.h"
#endif

int ElementOptSub(const float *in0, const float *in1, float *out, int size, const ArithmeticParameter *param) {
  int index = 0;
  if (param->in_elements_num0_ == 1) {
    SIMD_RUN_NO_SCALAR(ElementOptSubNum0, index, in0, in1, out, size);
    for (; index < size; index++) {
      out[index] = in0[0] - in1[index];
    }
  } else {
    SIMD_RUN_NO_SCALAR(ElementOptSubNum1, index, in0, in1, out, size);
    for (; index < size; index++) {
      out[index] = in0[index] - in1[0];
    }
  }
  return NNACL_OK;
}

int ElementOptSubInt(const int *in0, const int *in1, int *out, int size, const ArithmeticParameter *param) {
  int index = 0;
  if (param->in_elements_num0_ == 1) {
    SIMD_RUN_NO_SCALAR(ElementOptSubIntNum0, index, in0, in1, out, size);
    for (; index < size; index++) {
      out[index] = in0[0] - in1[index];
    }
  } else {
    SIMD_RUN_NO_SCALAR(ElementOptSubIntNum1, index, in0, in1, out, size);
    for (; index < size; index++) {
      out[index] = in0[index] - in1[0];
    }
  }
  return NNACL_OK;
}

int ElementOptSubRelu(const float *in0, const float *in1, float *out, int size, const ArithmeticParameter *param) {
  int index = 0;
  if (param->in_elements_num0_ == 1) {
    SIMD_RUN_NO_SCALAR(ElementOptSubReluNum0, index, in0, in1, out, size);
    for (; index < size; index++) {
      out[index] = MSMAX(in0[0] - in1[index], 0);
    }
  } else {
    SIMD_RUN_NO_SCALAR(ElementOptSubReluNum1, index, in0, in1, out, size);
    for (; index < size; index++) {
      out[index] = MSMAX(in0[index] - in1[0], 0);
    }
  }
  return NNACL_OK;
}

int ElementOptSubRelu6(const float *in0, const float *in1, float *out, int size, const ArithmeticParameter *param) {
  int index = 0;
  if (param->in_elements_num0_ == 1) {
    SIMD_RUN_NO_SCALAR(ElementOptSubRelu6Num0, index, in0, in1, out, size);
    for (; index < size; index++) {
      out[index] = MSMIN(MSMAX(in0[0] - in1[index], 0), 6);
    }
  } else {
    SIMD_RUN_NO_SCALAR(ElementOptSubRelu6Num1, index, in0, in1, out, size);
    for (; index < size; index++) {
      out[index] = MSMIN(MSMAX(in0[index] - in1[0], 0), 6);
    }
  }
  return NNACL_OK;
}

int ElementSub(const float *in0, const float *in1, float *out, int size) {
  int index = 0;

  SIMD_RUN_NO_SCALAR(ElementSub, index, in0, in1, out, size);
  for (; index < size; index++) {
    out[index] = in0[index] - in1[index];
  }
  return NNACL_OK;
}

int ElementSubInt(const int *in0, const int *in1, int *out, int size) {
  int index = 0;

  SIMD_RUN_NO_SCALAR(ElementSubInt, index, in0, in1, out, size);
  for (; index < size; index++) {
    out[index] = in0[index] - in1[index];
  }
  return NNACL_OK;
}

int ElementSubRelu(const float *in0, const float *in1, float *out, int size) {
  int index = 0;

  SIMD_RUN_NO_SCALAR(ElementSubRelu, index, in0, in1, out, size);
  for (; index < size; index++) {
    float res = in0[index] - in1[index];
    out[index] = res > 0 ? res : 0;
  }
  return NNACL_OK;
}

int ElementSubRelu6(const float *in0, const float *in1, float *out, int size) {
  int index = 0;

  SIMD_RUN_NO_SCALAR(ElementSubRelu6, index, in0, in1, out, size);
  for (; index < size; index++) {
    out[index] = MSMIN(MSMAX(in0[index] - in1[index], 0), 6);
  }

  return NNACL_OK;
}
