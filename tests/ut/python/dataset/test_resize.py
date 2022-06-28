# Copyright 2020-2022 Huawei Technologies Co., Ltd
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
# ==============================================================================
"""
Testing Resize op in DE
"""
import pytest
import mindspore.dataset as ds
import mindspore.dataset.vision as vision
from mindspore.dataset.vision.utils import Inter
from mindspore import log as logger
from util import visualize_list, save_and_check_md5, \
    config_get_set_seed, config_get_set_num_parallel_workers

DATA_DIR = ["../data/dataset/test_tf_file_3_images/train-0000-of-0001.data"]
SCHEMA_DIR = "../data/dataset/test_tf_file_3_images/datasetSchema.json"
GENERATE_GOLDEN = False


def test_resize_op(plot=False):
    """
    Feature: Resize op
    Description: Test Resize op basic usage
    Expectation: The dataset is processed as expected
    """
    def test_resize_op_parameters(test_name, size, plot):
        """
        Test resize_op
        """
        logger.info("Test resize: {0}".format(test_name))
        data1 = ds.TFRecordDataset(DATA_DIR, SCHEMA_DIR, columns_list=["image"], shuffle=False)

        # define map operations
        decode_op = vision.Decode()
        resize_op = vision.Resize(size)

        # apply map operations on images
        data1 = data1.map(operations=decode_op, input_columns=["image"])
        data2 = data1.map(operations=resize_op, input_columns=["image"])
        image_original = []
        image_resized = []
        for item1, item2 in zip(data1.create_dict_iterator(num_epochs=1, output_numpy=True),
                                data2.create_dict_iterator(num_epochs=1, output_numpy=True)):
            image_1 = item1["image"]
            image_2 = item2["image"]
            image_original.append(image_1)
            image_resized.append(image_2)
        if plot:
            visualize_list(image_original, image_resized)

    test_resize_op_parameters("Test single int for size", 10, plot=plot)
    test_resize_op_parameters("Test tuple for size", (10, 15), plot=plot)

def test_resize_op_ANTIALIAS():
    """
    Feature: Resize op
    Description: Test Resize op basic usage where image interpolation mode is Inter.ANTIALIAS
    Expectation: The dataset is processed as expected
    """
    logger.info("Test resize for ANTIALIAS")
    data1 = ds.TFRecordDataset(DATA_DIR, SCHEMA_DIR, columns_list=["image"], shuffle=False)

    # define map operations
    decode_op = vision.Decode(True)
    resize_op = vision.Resize(20, Inter.ANTIALIAS)

    # apply map operations on images
    data1 = data1.map(operations=[decode_op, resize_op, vision.ToTensor()], input_columns=["image"])

    num_iter = 0
    for _ in data1.create_dict_iterator(num_epochs=1, output_numpy=True):
        num_iter += 1
    logger.info("use Resize by Inter.ANTIALIAS process {} images.".format(num_iter))


def run_test_resize_md5(test_name, size, filename, seed, expected_size, to_pil=True, plot=False):
    """
    Run Resize with md5 check for python and C op versions
    """
    logger.info("Test Resize with md5 check: {0}".format(test_name))
    original_seed = config_get_set_seed(seed)
    original_num_parallel_workers = config_get_set_num_parallel_workers(1)
    # Generate dataset
    dataset = ds.TFRecordDataset(DATA_DIR, SCHEMA_DIR, columns_list=["image"], shuffle=False)
    compose_ops = ds.transforms.Compose([vision.Decode(to_pil=to_pil), vision.Resize(size)])
    transformed_data = dataset.map(operations=compose_ops, input_columns=["image"])
    # Compare with expected md5 from images
    save_and_check_md5(transformed_data, filename, generate_golden=GENERATE_GOLDEN)
    for item in transformed_data.create_dict_iterator(num_epochs=1, output_numpy=True):
        resized_image = item["image"]
        assert resized_image.shape == expected_size
    if plot:
        image_original = []
        image_resized = []
        original_data = dataset.map(operations=vision.Decode(), input_columns=["image"])
        for item1, item2 in zip(original_data.create_dict_iterator(num_epochs=1, output_numpy=True),
                                transformed_data.create_dict_iterator(num_epochs=1, output_numpy=True)):
            image_1 = item1["image"]
            image_2 = item2["image"]
            image_original.append(image_1)
            image_resized.append(image_2)
        visualize_list(image_original, image_resized)
    # Restore configuration
    ds.config.set_seed(original_seed)
    ds.config.set_num_parallel_workers(original_num_parallel_workers)


def test_resize_md5_c(plot=False):
    """
    Feature: Resize op C version
    Description: Test C Resize op using md5 check
    Expectation: Passes the md5 check test
    """
    run_test_resize_md5("Test single int for size", 5, "resize_01_result_c.npz",
                        5, (5, 8, 3), to_pil=False, plot=plot)
    run_test_resize_md5("Test tuple for size", (5, 7), "resize_02_result_c.npz",
                        7, (5, 7, 3), to_pil=False, plot=plot)


def test_resize_md5_py(plot=False):
    """
    Feature: Resize op py version
    Description: Test python Resize op using md5 check
    Expectation: Passes the md5 check test
    """
    run_test_resize_md5("Test single int for size", 5, "resize_01_result_py.npz",
                        5, (5, 8, 3), to_pil=True, plot=plot)
    run_test_resize_md5("Test tuple for size", (5, 7), "resize_02_result_py.npz",
                        7, (5, 7, 3), to_pil=True, plot=plot)


def test_resize_op_invalid_input():
    """
    Feature: Resize op
    Description: Test Resize op with invalid input
    Expectation: Correct error is raised as expected
    """
    def test_invalid_input(test_name, size, interpolation, error, error_msg):
        logger.info("Test Resize with bad input: {0}".format(test_name))
        with pytest.raises(error) as error_info:
            vision.Resize(size, interpolation)
        assert error_msg in str(error_info.value)

    test_invalid_input("invalid size parameter type as a single number", 4.5, Inter.LINEAR, TypeError,
                       "Size should be a single integer or a list/tuple (h, w) of length 2.")
    test_invalid_input("invalid size parameter shape", (2, 3, 4), Inter.LINEAR, TypeError,
                       "Size should be a single integer or a list/tuple (h, w) of length 2.")
    test_invalid_input("invalid size parameter type in a tuple", (2.3, 3), Inter.LINEAR, TypeError,
                       "Argument size at dim 0 with value 2.3 is not of type [<class 'int'>]")
    test_invalid_input("invalid Interpolation value", (2.3, 3), None, KeyError, "None")


if __name__ == "__main__":
    test_resize_op(plot=True)
    test_resize_op_ANTIALIAS()
    test_resize_md5_c(plot=False)
    test_resize_md5_py(plot=False)
    test_resize_op_invalid_input()
