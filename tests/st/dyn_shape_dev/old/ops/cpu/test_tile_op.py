# Copyright 2021 Huawei Technologies Co., Ltd
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

import mindspore.context as context
import mindspore.nn as nn
from mindspore import Tensor
from mindspore.ops import operations as P

context.set_context(mode=context.GRAPH_MODE, device_target="CPU")


class Net(nn.Cell):
    def __init__(self):
        super(Net, self).__init__()
        self.tile = P.Tile()

    def construct(self, x):
        return self.tile(x, (1, 4))


@pytest.mark.level0
@pytest.mark.platform_x86_cpu
@pytest.mark.env_onecard
def test_net():
    """
    Feature: Tile Op.
    Description: Tile with int32 input.
    Expectation: run success without error.
    """
    arr_x = np.array([[0], [1], [2], [3]]).astype(np.int32)
    tile = Net()
    print(arr_x)
    output = tile(Tensor(arr_x))
    print(output.asnumpy())


@pytest.mark.level0
@pytest.mark.platform_x86_cpu
@pytest.mark.env_onecard
def test_net_float64():
    """
    Feature: Tile Op.
    Description: Tile with float64 input.
    Expectation: run success without error.
    """
    arr_x = np.array([[0], [1], [2], [3]]).astype(np.float64)
    tile = Net()
    print(arr_x)
    output = tile(Tensor(arr_x))
    print(output.asnumpy())


@pytest.mark.level0
@pytest.mark.platform_x86_cpu
@pytest.mark.env_onecard
def test_net_bool1():
    """
    Feature: Tile Op.
    Description: Tile with bool input.
    Expectation: run success without error.
    """
    arr_x = np.array([[0], [1], [2], [3]]).astype(np.bool_)
    tile = Net()
    print(arr_x)
    output = tile(Tensor(arr_x))
    print(output.asnumpy())
