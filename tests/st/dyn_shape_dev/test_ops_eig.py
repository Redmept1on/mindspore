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

import pytest
import numpy as np
import mindspore as ms
from mindspore import ops, Tensor


@ms.jit
def eig_forward_func(input_x, compute_v):
    return ops.auto_generate.eig_(input_x, compute_v)


@pytest.mark.level0
@pytest.mark.env_onecard
@pytest.mark.platform_x86_cpu
def test_eig_forward():
    """
    Feature: Ops.
    Description: test op eig.
    Expectation: expect correct result.
    """
    input_x = Tensor(np.array([[1.0, 0.0], [0.0, 2.0]]), ms.float32)
    compute_v = True
    u, v = eig_forward_func(input_x, compute_v)
    expect_u = [1.+0.j, 2.+0.j]
    expect_v = [[1.+0.j, 0.+0.j], [0.+0.j, 1.+0.j]]
    assert np.allclose(u.asnumpy(), expect_u, rtol=0.0001)
    assert np.allclose(v.asnumpy(), expect_v, rtol=0.0001)
