# Copyright 2023 Huawei Technologies Co., Ltd
#
# Licensed under the Apache License, Version 2.0 (the "License");
# you may not use this file except in compliance with the License.
# You may obtain a copy of the License at
#
# http://www.apache.org/licenses/LICENSE-2.0
#
# Unless required by applicable law or agreed to in writing, software
# distributed under the License is distributed on an "AS IS" BASIS,
# WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
# See the License for the specific language governing permissions and
# limitations under the License.
# ============================================================================

import numpy as np
import pytest
import mindspore as ms
from mindspore import ops
from mindspore.ops.auto_generate.gen_ops_def import _relu_grad


@ms.jit
def relu_grad_func(dy, x):
    return _relu_grad(dy, x)


@pytest.mark.level0
@pytest.mark.env_onecard
@pytest.mark.platform_x86_cpu
@pytest.mark.platform_x86_gpu_training
@pytest.mark.platform_arm_ascend_training
def test_relu_grad():
    """
    Feature: Ops.
    Description: test op relu grad.
    Expectation: expect correct result.
    """
    dy = ms.Tensor(np.array([[[[1, 0, 1],
                               [0, 1, 0],
                               [1, 1, 1]]]]).astype(np.float32))
    x = ms.Tensor(np.array([[[[-1, 1, 1],
                              [1, -1, 1],
                              [1, 1, -1]]]]).astype(np.float32))
    out = relu_grad_func(dy, x)
    expect_out = np.array([[[[0, 0, 1],
                             [0, 0, 0],
                             [1, 1, 0]]]]).astype(np.float32)
    assert (out.asnumpy() == expect_out).all()


@pytest.mark.level0
@pytest.mark.env_onecard
@pytest.mark.platform_x86_cpu
@pytest.mark.platform_x86_gpu_training
def test_relu_grad_vmap():
    """
    Feature: test vmap function.
    Description: test relu op vmap.
    Expectation: expect correct result.
    """
    axes = (-1, -1)
    dy = ms.Tensor(np.random.rand(4, 3, 2).astype(np.float32))
    x = ms.Tensor(np.random.randint(low=-10, high=10, size=(4, 3, 2)).astype(np.float32)) * 0.1
    net_vmap = ops.vmap(ops.vmap(relu_grad_func, in_axes=axes, out_axes=-1), in_axes=axes, out_axes=-1)
    out = net_vmap(dy, x)
    expect_out = relu_grad_func(dy, x)
    assert (out.asnumpy() == expect_out.asnumpy()).all()
