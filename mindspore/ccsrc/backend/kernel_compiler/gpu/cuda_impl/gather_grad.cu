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

#include <iostream>
#include "backend/kernel_compiler/gpu/cuda_impl/gather_grad.cuh"
#include "backend/kernel_compiler/gpu/cuda_impl/util.cuh"
#include "runtime/device/gpu/cuda_common.h"

template <typename T, typename S>
__global__ void GatherGradKernel(const size_t num, const T *index, const S *grad, S *output,
                                 const size_t dim_before_axis, const size_t dim_at_axis_index,
                                 const size_t dim_at_axis_output, const size_t dim_after_axis) {
  size_t i, k;

  for (size_t id = blockIdx.x * blockDim.x + threadIdx.x; id < num;
       id += blockDim.x * gridDim.x) {
    i = id / (dim_at_axis_index * dim_after_axis);
    k = id % dim_after_axis;

    size_t j_read = static_cast<size_t>(index[id]);
    size_t read_id = i * dim_at_axis_output * dim_after_axis + j_read * dim_after_axis + k;
    MsAtomicAdd(output + read_id, grad[id]);
  }
  return;
}

template <typename S>
__global__ void InitOutput(const size_t size, S *output) {
    S zero = 0;
    for (size_t id = blockIdx.x * blockDim.x + threadIdx.x; id < size; id += blockDim.x * gridDim.x) {
        output[id] = zero;
    }
    return;
}

template <typename T, typename S>
void GatherGrad(const T *index, const S *grad, S *output, const size_t dim_before_axis,
                const size_t dim_at_axis_index, const size_t dim_at_axis_output, const size_t dim_after_axis,
                cudaStream_t stream) {
  size_t size = dim_before_axis * dim_at_axis_output * dim_after_axis;
  InitOutput<<<GET_BLOCKS(size), GET_THREADS, 0, stream>>>(size, output);

  size = dim_before_axis * dim_at_axis_index * dim_after_axis;
  GatherGradKernel<<<GET_BLOCKS(size), GET_THREADS, 0, stream>>>(size, index, grad, output,
                                                                 dim_before_axis, dim_at_axis_index,
                                                                 dim_at_axis_output, dim_after_axis);
  return;
}

template void GatherGrad<int, double>(const int *index, const double *grad, double *output,
                                     const size_t dim_before_axis, const size_t dim_at_axis_index,
                                     const size_t dim_at_axis_output, const size_t dim_after_axis,
                                     cudaStream_t stream);
template void GatherGrad<int64_t, double>(const int64_t *index, const double *grad, double *output,
                                          const size_t dim_before_axis, const size_t dim_at_axis_index,
                                          const size_t dim_at_axis_output, const size_t dim_after_axis,
                                          cudaStream_t stream);
template void GatherGrad<int, float>(const int *index, const float *grad, float *output,
                                     const size_t dim_before_axis, const size_t dim_at_axis_index,
                                     const size_t dim_at_axis_output, const size_t dim_after_axis,
                                     cudaStream_t stream);
template void GatherGrad<int64_t, float>(const int64_t *index, const float *grad, float *output,
                                         const size_t dim_before_axis, const size_t dim_at_axis_index,
                                         const size_t dim_at_axis_output, const size_t dim_after_axis,
                                         cudaStream_t stream);
template void GatherGrad<int, half>(const int *index, const half *grad, half *output,
                                    const size_t dim_before_axis, const size_t dim_at_axis_index,
                                    const size_t dim_at_axis_output, const size_t dim_after_axis,
                                    cudaStream_t stream);
template void GatherGrad<int64_t, half>(const int64_t *index, const half *grad, half *output,
                                        const size_t dim_before_axis, const size_t dim_at_axis_index,
                                        const size_t dim_at_axis_output, const size_t dim_after_axis,
                                        cudaStream_t stream);
