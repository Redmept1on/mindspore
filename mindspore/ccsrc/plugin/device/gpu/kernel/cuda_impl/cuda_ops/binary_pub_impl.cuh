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
 * WITH WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 * See the License for the specific language governing permissions and
 * limitations under the License.
 */
#ifndef MINDSPORE_CCSRC_PLUGIN_DEVICE_GPU_KERNEL_CUDA_IMPL_CUDA_OPS_BINARY_PUB_CUH_
#define MINDSPORE_CCSRC_PLUGIN_DEVICE_GPU_KERNEL_CUDA_IMPL_CUDA_OPS_BINARY_PUB_CUH_
#include <math.h>
#include <vector>
#include <iostream>
#include "include/cuda_fp16.h"
#include "plugin/device/gpu/kernel/cuda_impl/cuda_ops/binary_types.cuh"
#include "plugin/device/gpu/kernel/cuda_impl/cuda_ops/binary_common.cuh"
#include "plugin/device/gpu/kernel/cuda_impl/cuda_ops/elementwise/elementswise_pub_impl.cuh"

constexpr uint kThreadsPerBlock = 512;

template <typename T, size_t VecSize>
struct Vec {
  T data[VecSize];
};

struct BinaryBroadcastStrideInfo {
  size_t in0_stride[8];
  size_t in1_stride[8];
  size_t out_stride[8];
};

enum class ScalarOption {
  NoScalar = 0,
  In0Scalar = 1,
  In1Scalar = 2,
};

static __device__ Vec<size_t, 2> CalInPosByOutPos(size_t out_pos, size_t dim_size,
                                                  const BinaryBroadcastStrideInfo &strides) {
  Vec<size_t, 2> in_pos = {0, 0};
  size_t tmp_idx = 0;
  for (int idx = 0; idx < dim_size; ++idx) {
    tmp_idx = out_pos / strides.out_stride[idx];
    in_pos.data[0] += tmp_idx * strides.in0_stride[idx];
    in_pos.data[1] += tmp_idx * strides.in1_stride[idx];
    out_pos -= tmp_idx * strides.out_stride[idx];
  }
  return in_pos;
}

template <BinaryOpType OP, typename In0_t, typename In1_t, typename Out_t>
__global__ void BinaryWithBroadcastKernel(BinaryFunc<OP, In0_t, In1_t, Out_t> func, size_t dim_size, size_t out_num,
                                          BinaryBroadcastStrideInfo strides, In0_t *in0, In1_t *in1, Out_t *out) {
  for (size_t pos = blockIdx.x * blockDim.x + threadIdx.x; pos < out_num; pos += blockDim.x * gridDim.x) {
    Vec<size_t, 2> in_pos = CalInPosByOutPos(pos, dim_size, strides);
    out[pos] = func(in0[in_pos.data[0]], in1[in_pos.data[1]]);
  }
}

template <BinaryOpType OP, typename In0_t, typename In1_t, typename Out_t>
__global__ void BinaryWithoutBroadcastIn0Scalar(BinaryFunc<OP, In0_t, In1_t, Out_t> func, size_t out_num, In0_t *in0,
                                                In1_t *in1, Out_t *out) {
  In0_t in0_scalar = in0[0];
  for (size_t pos = blockIdx.x * blockDim.x + threadIdx.x; pos < out_num; pos += blockDim.x * gridDim.x) {
    out[pos] = func(in0_scalar, in1[pos]);
  }
}

template <BinaryOpType OP, typename In0_t, typename In1_t, typename Out_t>
__global__ void BinaryWithoutBroadcastIn1Scalar(BinaryFunc<OP, In0_t, In1_t, Out_t> func, size_t out_num, In0_t *in0,
                                                In1_t *in1, Out_t *out) {
  In1_t in1_scalar = in1[0];
  for (size_t pos = blockIdx.x * blockDim.x + threadIdx.x; pos < out_num; pos += blockDim.x * gridDim.x) {
    out[pos] = func(in0[pos], in1_scalar);
  }
}

template <BinaryOpType OP, typename In0_t, typename In1_t, typename Out_t>
__global__ void BinaryWithoutBroadcastNoScalar(BinaryFunc<OP, In0_t, In1_t, Out_t> func, size_t out_num, In0_t *in0,
                                               In1_t *in1, Out_t *out) {
  for (size_t pos = blockIdx.x * blockDim.x + threadIdx.x; pos < out_num; pos += blockDim.x * gridDim.x) {
    out[pos] = func(in0[pos], in1[pos]);
  }
}

static BinaryBroadcastStrideInfo BinaryBroadcastCalStride(const size_t dim_size, const std::vector<int64_t> &in0_shape,
                                                          const std::vector<int64_t> &in1_shape,
                                                          const std::vector<int64_t> &out_shape) {
  BinaryBroadcastStrideInfo strides;
  strides.in0_stride[dim_size - 1] = 1;
  strides.in1_stride[dim_size - 1] = 1;
  strides.out_stride[dim_size - 1] = 1;
  for (int64_t idx = dim_size - 2; idx >= 0; --idx) {
    strides.out_stride[idx] = out_shape[idx + 1] * strides.out_stride[idx + 1];
    strides.in0_stride[idx] = in0_shape[idx + 1] * strides.in0_stride[idx + 1];
    strides.in1_stride[idx] = in1_shape[idx + 1] * strides.in1_stride[idx + 1];
  }
  for (size_t idx = 0; idx < dim_size; ++idx) {
    strides.in0_stride[idx] = (in0_shape[idx] == 1) ? 0 : strides.in0_stride[idx];
    strides.in1_stride[idx] = (in1_shape[idx] == 1) ? 0 : strides.in1_stride[idx];
  }
  return strides;
}

template <BinaryOpType OP, typename In0_t, typename In1_t, typename Out_t>
cudaError_t BinaryWithBroadcast(BinaryFunc<OP, In0_t, In1_t, Out_t> func, size_t out_num,
                                const std::vector<int64_t> &in0_shape, const std::vector<int64_t> &in1_shape,
                                const std::vector<int64_t> &out_shape, In0_t *in0, In1_t *in1, Out_t *out,
                                size_t device_id, cudaStream_t cuda_stream) {
  const size_t dim_size = out_shape.size();
  BinaryBroadcastStrideInfo strides = BinaryBroadcastCalStride(dim_size, in0_shape, in1_shape, out_shape);
  size_t thread_num = out_num > kThreadsPerBlock ? kThreadsPerBlock : out_num;
  BinaryWithBroadcastKernel<OP, In0_t, In1_t, Out_t>
    <<<CUDA_BLOCKS_CAL(device_id, out_num, thread_num), thread_num, 0, cuda_stream>>>(func, dim_size, out_num, strides,
                                                                                      in0, in1, out);
  CHECK_CUDA_LAUNCH_SUCCESS();
}

template <BinaryOpType OP, typename In0_t, typename In1_t, typename Out_t>
cudaError_t BinaryWithoutBroadcast(BinaryFunc<OP, In0_t, In1_t, Out_t> func, ScalarOption scalar_option, size_t out_num,
                                   Out_t *out, In0_t *in0, In1_t *in1, size_t device_id, cudaStream_t cuda_stream) {
  size_t thread_num = out_num > kThreadsPerBlock ? kThreadsPerBlock : out_num;
  if (scalar_option == ScalarOption::In0Scalar) {
    BinaryWithoutBroadcastIn0Scalar<OP, In0_t, In1_t, Out_t>
      <<<CUDA_BLOCKS_CAL(device_id, out_num, thread_num), thread_num, 0, cuda_stream>>>(func, out_num, in0, in1, out);
  } else if (scalar_option == ScalarOption::In1Scalar) {
    BinaryWithoutBroadcastIn1Scalar<OP, In0_t, In1_t, Out_t>
      <<<CUDA_BLOCKS_CAL(device_id, out_num, thread_num), thread_num, 0, cuda_stream>>>(func, out_num, in0, in1, out);
  } else {
    if (out_num < 32 * kThreadsPerBlock) {
      BinaryWithoutBroadcastNoScalar<OP, In0_t, In1_t, Out_t>
        <<<CUDA_BLOCKS_CAL(device_id, out_num, thread_num), thread_num, 0, cuda_stream>>>(func, out_num, in0, in1, out);
    } else {
      cuda::elementwise::Binary(func, out_num, out, in0, in1, cuda_stream);
    }
  }
  CHECK_CUDA_LAUNCH_SUCCESS();
}

template <enum BinaryOpType OP, typename In0_t, typename In1_t, typename Out_t>
cudaError_t BinaryOpWithBroadcastCudaFunc(const bool is_broadcast, const std::vector<int64_t> &in0_shape,
                                          const std::vector<int64_t> &in1_shape, const std::vector<int64_t> &out_shape,
                                          In0_t *in0, In1_t *in1, Out_t *out, size_t device_id,
                                          cudaStream_t cuda_stream) {
  BinaryFunc<OP, In0_t, In1_t, Out_t> func;
  size_t out_num = 1;
  for (auto val : out_shape) {
    out_num *= val;
  }
  if (is_broadcast) {
    return BinaryWithBroadcast<OP, In0_t, In1_t, Out_t>(func, out_num, in0_shape, in1_shape, out_shape, in0, in1, out,
                                                        device_id, cuda_stream);
  } else {
    ScalarOption scalar_option = ScalarOption::NoScalar;
    if (in0_shape.size() == 1 && in0_shape[0] == 1) {
      scalar_option = ScalarOption::In0Scalar;
    }
    if (in1_shape.size() == 1 && in1_shape[0] == 1) {
      scalar_option = ScalarOption::In1Scalar;
    }
    return BinaryWithoutBroadcast<OP, In0_t, In1_t, Out_t>(func, scalar_option, out_num, out, in0, in1, device_id,
                                                           cuda_stream);
  }
}

#define REGISTER_BINARY_OP_CUDA_FUNC_BOOL_TYPE(op)                                                         \
  template CUDA_LIB_EXPORT cudaError_t BinaryOpWithBroadcastCudaFunc<op, bool, bool, bool>(                \
    const bool is_broadcast, const std::vector<int64_t> &in0_shape, const std::vector<int64_t> &in1_shape, \
    const std::vector<int64_t> &out_shape, bool *in0, bool *in1, bool *out, size_t device_id,              \
    cudaStream_t cuda_stream);

#define REGISTER_BINARY_OP_CUDA_FUNC_INT_TYPE(op)                                                          \
  template CUDA_LIB_EXPORT cudaError_t BinaryOpWithBroadcastCudaFunc<op, int8_t, int8_t, int8_t>(          \
    const bool is_broadcast, const std::vector<int64_t> &in0_shape, const std::vector<int64_t> &in1_shape, \
    const std::vector<int64_t> &out_shape, int8_t *in0, int8_t *in1, int8_t *out, size_t device_id,        \
    cudaStream_t cuda_stream);                                                                             \
  template CUDA_LIB_EXPORT cudaError_t BinaryOpWithBroadcastCudaFunc<op, uint8_t, uint8_t, uint8_t>(       \
    const bool is_broadcast, const std::vector<int64_t> &in0_shape, const std::vector<int64_t> &in1_shape, \
    const std::vector<int64_t> &out_shape, uint8_t *in0, uint8_t *in1, uint8_t *out, size_t device_id,     \
    cudaStream_t cuda_stream);                                                                             \
  template CUDA_LIB_EXPORT cudaError_t BinaryOpWithBroadcastCudaFunc<op, int16_t, int16_t, int16_t>(       \
    const bool is_broadcast, const std::vector<int64_t> &in0_shape, const std::vector<int64_t> &in1_shape, \
    const std::vector<int64_t> &out_shape, int16_t *in0, int16_t *in1, int16_t *out, size_t device_id,     \
    cudaStream_t cuda_stream);                                                                             \
  template CUDA_LIB_EXPORT cudaError_t BinaryOpWithBroadcastCudaFunc<op, uint16_t, uint16_t, uint16_t>(    \
    const bool is_broadcast, const std::vector<int64_t> &in0_shape, const std::vector<int64_t> &in1_shape, \
    const std::vector<int64_t> &out_shape, uint16_t *in0, uint16_t *in1, uint16_t *out, size_t device_id,  \
    cudaStream_t cuda_stream);                                                                             \
  template CUDA_LIB_EXPORT cudaError_t BinaryOpWithBroadcastCudaFunc<op, int32_t, int32_t, int32_t>(       \
    const bool is_broadcast, const std::vector<int64_t> &in0_shape, const std::vector<int64_t> &in1_shape, \
    const std::vector<int64_t> &out_shape, int32_t *in0, int32_t *in1, int32_t *out, size_t device_id,     \
    cudaStream_t cuda_stream);                                                                             \
  template CUDA_LIB_EXPORT cudaError_t BinaryOpWithBroadcastCudaFunc<op, uint32_t, uint32_t, uint32_t>(    \
    const bool is_broadcast, const std::vector<int64_t> &in0_shape, const std::vector<int64_t> &in1_shape, \
    const std::vector<int64_t> &out_shape, uint32_t *in0, uint32_t *in1, uint32_t *out, size_t device_id,  \
    cudaStream_t cuda_stream);                                                                             \
  template CUDA_LIB_EXPORT cudaError_t BinaryOpWithBroadcastCudaFunc<op, int64_t, int64_t, int64_t>(       \
    const bool is_broadcast, const std::vector<int64_t> &in0_shape, const std::vector<int64_t> &in1_shape, \
    const std::vector<int64_t> &out_shape, int64_t *in0, int64_t *in1, int64_t *out, size_t device_id,     \
    cudaStream_t cuda_stream);                                                                             \
  template CUDA_LIB_EXPORT cudaError_t BinaryOpWithBroadcastCudaFunc<op, uint64_t, uint64_t, uint64_t>(    \
    const bool is_broadcast, const std::vector<int64_t> &in0_shape, const std::vector<int64_t> &in1_shape, \
    const std::vector<int64_t> &out_shape, uint64_t *in0, uint64_t *in1, uint64_t *out, size_t device_id,  \
    cudaStream_t cuda_stream)

#define REGISTER_BINARY_OP_CUDA_FUNC_FLOAT_TYPE(op)                                                        \
  template CUDA_LIB_EXPORT cudaError_t BinaryOpWithBroadcastCudaFunc<op, double, double, double>(          \
    const bool is_broadcast, const std::vector<int64_t> &in0_shape, const std::vector<int64_t> &in1_shape, \
    const std::vector<int64_t> &out_shape, double *in0, double *in1, double *out, size_t device_id,        \
    cudaStream_t cuda_stream);                                                                             \
  template CUDA_LIB_EXPORT cudaError_t BinaryOpWithBroadcastCudaFunc<op, float, float, float>(             \
    const bool is_broadcast, const std::vector<int64_t> &in0_shape, const std::vector<int64_t> &in1_shape, \
    const std::vector<int64_t> &out_shape, float *in0, float *in1, float *out, size_t device_id,           \
    cudaStream_t cuda_stream);                                                                             \
  template CUDA_LIB_EXPORT cudaError_t BinaryOpWithBroadcastCudaFunc<op, half, half, half>(                \
    const bool is_broadcast, const std::vector<int64_t> &in0_shape, const std::vector<int64_t> &in1_shape, \
    const std::vector<int64_t> &out_shape, half *in0, half *in1, half *out, size_t device_id,              \
    cudaStream_t cuda_stream);

#define REGISTER_BINARY_OP_CUDA_FUNC_COMPLEX_TYPE(op)                                                                 \
  template CUDA_LIB_EXPORT cudaError_t                                                                                \
  BinaryOpWithBroadcastCudaFunc<op, Complex<float>, Complex<float>, Complex<float>>(                                  \
    const bool is_broadcast, const std::vector<int64_t> &in0_shape, const std::vector<int64_t> &in1_shape,            \
    const std::vector<int64_t> &out_shape, Complex<float> *in0, Complex<float> *in1, Complex<float> *out,             \
    size_t device_id, cudaStream_t cuda_stream);                                                                      \
  template CUDA_LIB_EXPORT cudaError_t                                                                                \
  BinaryOpWithBroadcastCudaFunc<op, Complex<double>, Complex<double>, Complex<double>>(                               \
    const bool is_broadcast, const std::vector<int64_t> &in0_shape, const std::vector<int64_t> &in1_shape,            \
    const std::vector<int64_t> &out_shape, Complex<double> *in0, Complex<double> *in1, Complex<double> *out,          \
    size_t device_id, cudaStream_t cuda_stream);                                                                      \
  template CUDA_LIB_EXPORT cudaError_t BinaryOpWithBroadcastCudaFunc<op, Complex<float>, float, Complex<float>>(      \
    const bool is_broadcast, const std::vector<int64_t> &in0_shape, const std::vector<int64_t> &in1_shape,            \
    const std::vector<int64_t> &out_shape, Complex<float> *in0, float *in1, Complex<float> *out, size_t device_id,    \
    cudaStream_t cuda_stream);                                                                                        \
  template CUDA_LIB_EXPORT cudaError_t BinaryOpWithBroadcastCudaFunc<op, float, Complex<float>, Complex<float>>(      \
    const bool is_broadcast, const std::vector<int64_t> &in0_shape, const std::vector<int64_t> &in1_shape,            \
    const std::vector<int64_t> &out_shape, float *in0, Complex<float> *in1, Complex<float> *out, size_t device_id,    \
    cudaStream_t cuda_stream);                                                                                        \
  template CUDA_LIB_EXPORT cudaError_t BinaryOpWithBroadcastCudaFunc<op, double, Complex<double>, Complex<double>>(   \
    const bool is_broadcast, const std::vector<int64_t> &in0_shape, const std::vector<int64_t> &in1_shape,            \
    const std::vector<int64_t> &out_shape, double *in0, Complex<double> *in1, Complex<double> *out, size_t device_id, \
    cudaStream_t cuda_stream);                                                                                        \
  template CUDA_LIB_EXPORT cudaError_t BinaryOpWithBroadcastCudaFunc<op, Complex<double>, double, Complex<double>>(   \
    const bool is_broadcast, const std::vector<int64_t> &in0_shape, const std::vector<int64_t> &in1_shape,            \
    const std::vector<int64_t> &out_shape, Complex<double> *in0, double *in1, Complex<double> *out, size_t device_id, \
    cudaStream_t cuda_stream)
#endif
