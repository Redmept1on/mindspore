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
 * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 * See the License for the specific language governing permissions and
 * limitations under the License.
 */
#include "pybind11/pybind11.h"
#include "pybind11/stl.h"
#include "pybind11/stl_bind.h"

#include "minddata/dataset/api/python/pybind_conversion.h"
#include "minddata/dataset/api/python/pybind_register.h"
#include "minddata/dataset/callback/py_ds_callback.h"
#include "minddata/dataset/include/constants.h"
#include "minddata/dataset/core/global_context.h"
#include "minddata/dataset/engine/serdes.h"
#include "minddata/dataset/include/datasets.h"
#include "minddata/dataset/text/sentence_piece_vocab.h"

// IR non-leaf nodes
#include "minddata/dataset/engine/ir/datasetops/batch_node.h"
#include "minddata/dataset/engine/ir/datasetops/concat_node.h"
#include "minddata/dataset/engine/ir/datasetops/dataset_node.h"
#include "minddata/dataset/engine/ir/datasetops/filter_node.h"
#include "minddata/dataset/engine/ir/datasetops/map_node.h"
#include "minddata/dataset/engine/ir/datasetops/project_node.h"
#include "minddata/dataset/engine/ir/datasetops/rename_node.h"
#include "minddata/dataset/engine/ir/datasetops/repeat_node.h"
#include "minddata/dataset/engine/ir/datasetops/shuffle_node.h"
#include "minddata/dataset/engine/ir/datasetops/skip_node.h"
#include "minddata/dataset/engine/ir/datasetops/take_node.h"
#include "minddata/dataset/engine/ir/datasetops/transfer_node.h"
#include "minddata/dataset/engine/ir/datasetops/zip_node.h"

// IR non-leaf nodes - for android
#ifndef ENABLE_ANDROID
#include "minddata/dataset/engine/ir/datasetops/bucket_batch_by_length_node.h"
#include "minddata/dataset/engine/ir/datasetops/build_vocab_node.h"
#include "minddata/dataset/engine/ir/datasetops/build_sentence_piece_vocab_node.h"
#include "minddata/dataset/engine/ir/datasetops/sync_wait_node.h"
#endif

#include "minddata/dataset/core/config_manager.h"
#include "minddata/dataset/core/data_type.h"
#include "minddata/dataset/util/path.h"
#include "minddata/dataset/util/random.h"
#include "minddata/dataset/util/services.h"

namespace mindspore {
namespace dataset {

PYBIND_REGISTER(DatasetNode, 1, ([](const py::module *m) {
                  (void)py::class_<DatasetNode, std::shared_ptr<DatasetNode>>(*m, "Dataset")
                    .def("set_num_workers",
                         [](std::shared_ptr<DatasetNode> self, std::optional<int32_t> num_workers) {
                           return num_workers ? self->SetNumWorkers(*num_workers) : self;
                         })
                    .def("set_cache_client",
                         [](std::shared_ptr<DatasetNode> self, std::shared_ptr<CacheClient> cc) {
                           return self->SetDatasetCache(toDatasetCache(std::move(cc)));
                         })
                    .def(
                      "Zip",
                      [](std::shared_ptr<DatasetNode> self, py::list datasets) {
                        auto zip = std::make_shared<ZipNode>(std::move(toDatasetNode(self, datasets)));
                        THROW_IF_ERROR(zip->ValidateParams());
                        return zip;
                      },
                      py::arg("datasets"))
                    .def("to_json", [](std::shared_ptr<DatasetNode> self, const std::string &json_filepath) {
                      nlohmann::json args;
                      auto serdas = std::make_shared<Serdes>();
                      THROW_IF_ERROR(serdas->SaveToJSON(self, json_filepath, &args));
                      return args.dump();
                    });
                }));

// PYBIND FOR NON-LEAF NODES
// (In alphabetical order)

PYBIND_REGISTER(BatchNode, 2, ([](const py::module *m) {
                  (void)py::class_<BatchNode, DatasetNode, std::shared_ptr<BatchNode>>(*m, "BatchNode",
                                                                                       "to create a BatchNode")
                    .def(py::init([](std::shared_ptr<DatasetNode> self, int32_t batch_size, bool drop_remainder,
                                     bool pad, py::list in_col_names, py::list out_col_names, py::list col_order,
                                     py::object size_obj, py::object map_obj, py::dict pad_info) {
                      std::map<std::string, std::pair<TensorShape, std::shared_ptr<Tensor>>> c_pad_info;
                      if (pad) {
                        THROW_IF_ERROR(toPadInfo(pad_info, &c_pad_info));
                      }
                      py::function size_func =
                        py::isinstance<py::function>(size_obj) ? size_obj.cast<py::function>() : py::function();
                      py::function map_func =
                        py::isinstance<py::function>(map_obj) ? map_obj.cast<py::function>() : py::function();
                      auto batch = std::make_shared<BatchNode>(
                        self, batch_size, drop_remainder, pad, toStringVector(in_col_names),
                        toStringVector(out_col_names), toStringVector(col_order), size_func, map_func, c_pad_info);
                      THROW_IF_ERROR(batch->ValidateParams());
                      return batch;
                    }));
                }));

PYBIND_REGISTER(BucketBatchByLengthNode, 2, ([](const py::module *m) {
                  (void)py::class_<BucketBatchByLengthNode, DatasetNode, std::shared_ptr<BucketBatchByLengthNode>>(
                    *m, "BucketBatchByLengthNode", "to create a BucketBatchByLengthNode")
                    .def(py::init([](std::shared_ptr<DatasetNode> dataset, py::list column_names,
                                     std::vector<int32_t> bucket_boundaries, std::vector<int32_t> bucket_batch_sizes,
                                     py::object element_length_function, py::dict pad_info, bool pad_to_bucket_boundary,
                                     bool drop_remainder) {
                           std::map<std::string, std::pair<TensorShape, std::shared_ptr<Tensor>>> c_pad_info;
                           THROW_IF_ERROR(toPadInfo(pad_info, &c_pad_info));

                           auto bucket_batch = std::make_shared<BucketBatchByLengthNode>(
                             dataset, toStringVector(column_names), bucket_boundaries, bucket_batch_sizes,
                             toPyFuncOp(std::move(element_length_function), DataType::DE_INT32), c_pad_info,
                             pad_to_bucket_boundary, drop_remainder);
                           THROW_IF_ERROR(bucket_batch->ValidateParams());
                           return bucket_batch;
                         }),
                         py::arg("dataset"), py::arg("column_names"), py::arg("bucket_boundaries"),
                         py::arg("bucket_batch_sizes"), py::arg("element_length_function") = py::none(),
                         py::arg("pad_info"), py::arg("pad_to_bucket_boundary"), py::arg("drop_remainder"));
                }));

PYBIND_REGISTER(BuildSentenceVocabNode, 2, ([](const py::module *m) {
                  (void)py::class_<BuildSentenceVocabNode, DatasetNode, std::shared_ptr<BuildSentenceVocabNode>>(
                    *m, "BuildSentenceVocabNode", "to create a BuildSentenceVocabNode")
                    .def(py::init([](std::shared_ptr<DatasetNode> self, std::shared_ptr<SentencePieceVocab> vocab,
                                     const std::vector<std::string> &col_names, int32_t vocab_size,
                                     float character_coverage, SentencePieceModel model_type,
                                     const std::unordered_map<std::string, std::string> &params) {
                      auto build_sentence_vocab = std::make_shared<BuildSentenceVocabNode>(
                        self, vocab, col_names, vocab_size, character_coverage, model_type, params);
                      THROW_IF_ERROR(build_sentence_vocab->ValidateParams());
                      return build_sentence_vocab;
                    }));
                }));

PYBIND_REGISTER(BuildVocabNode, 2, ([](const py::module *m) {
                  (void)py::class_<BuildVocabNode, DatasetNode, std::shared_ptr<BuildVocabNode>>(
                    *m, "BuildVocabNode", "to create a BuildVocabNode")
                    .def(py::init([](std::shared_ptr<DatasetNode> self, std::shared_ptr<Vocab> vocab, py::list columns,
                                     py::tuple freq_range, int64_t top_k, py::list special_tokens, bool special_first) {
                      auto build_vocab =
                        std::make_shared<BuildVocabNode>(self, vocab, toStringVector(columns), toIntPair(freq_range),
                                                         top_k, toStringVector(special_tokens), special_first);
                      THROW_IF_ERROR(build_vocab->ValidateParams());
                      return build_vocab;
                    }));
                }));

PYBIND_REGISTER(ConcatNode, 2, ([](const py::module *m) {
                  (void)py::class_<ConcatNode, DatasetNode, std::shared_ptr<ConcatNode>>(*m, "ConcatNode",
                                                                                         "to create a ConcatNode")
                    .def(py::init([](std::vector<std::shared_ptr<DatasetNode>> datasets, py::handle sampler,
                                     py::list children_flag_and_nums, py::list children_start_end_index) {
                      auto concat = std::make_shared<ConcatNode>(datasets, toSamplerObj(sampler),
                                                                 toPairVector(children_flag_and_nums),
                                                                 toPairVector(children_start_end_index));
                      THROW_IF_ERROR(concat->ValidateParams());
                      return concat;
                    }));
                }));

PYBIND_REGISTER(FilterNode, 2, ([](const py::module *m) {
                  (void)py::class_<FilterNode, DatasetNode, std::shared_ptr<FilterNode>>(*m, "FilterNode",
                                                                                         "to create a FilterNode")
                    .def(py::init([](std::shared_ptr<DatasetNode> self, py::object predicate,
                                     std::vector<std::string> input_columns) {
                      auto filter =
                        std::make_shared<FilterNode>(self, toPyFuncOp(predicate, DataType::DE_BOOL), input_columns);
                      THROW_IF_ERROR(filter->ValidateParams());
                      return filter;
                    }));
                }));

PYBIND_REGISTER(MapNode, 2, ([](const py::module *m) {
                  (void)py::class_<MapNode, DatasetNode, std::shared_ptr<MapNode>>(*m, "MapNode", "to create a MapNode")
                    .def(py::init([](std::shared_ptr<DatasetNode> self, py::list operations, py::list input_columns,
                                     py::list output_columns, py::list project_columns,
                                     std::vector<std::shared_ptr<PyDSCallback>> py_callbacks) {
                      auto map = std::make_shared<MapNode>(
                        self, std::move(toTensorOperations(operations)), toStringVector(input_columns),
                        toStringVector(output_columns), toStringVector(project_columns), nullptr,
                        std::vector<std::shared_ptr<DSCallback>>(py_callbacks.begin(), py_callbacks.end()));
                      THROW_IF_ERROR(map->ValidateParams());
                      return map;
                    }));
                }));

PYBIND_REGISTER(ProjectNode, 2, ([](const py::module *m) {
                  (void)py::class_<ProjectNode, DatasetNode, std::shared_ptr<ProjectNode>>(*m, "ProjectNode",
                                                                                           "to create a ProjectNode")
                    .def(py::init([](std::shared_ptr<DatasetNode> self, py::list columns) {
                      auto project = std::make_shared<ProjectNode>(self, toStringVector(columns));
                      THROW_IF_ERROR(project->ValidateParams());
                      return project;
                    }));
                }));

PYBIND_REGISTER(
  RenameNode, 2, ([](const py::module *m) {
    (void)py::class_<RenameNode, DatasetNode, std::shared_ptr<RenameNode>>(*m, "RenameNode", "to create a RenameNode")
      .def(py::init([](std::shared_ptr<DatasetNode> self, py::list input_columns, py::list output_columns) {
        auto rename = std::make_shared<RenameNode>(self, toStringVector(input_columns), toStringVector(output_columns));
        THROW_IF_ERROR(rename->ValidateParams());
        return rename;
      }));
  }));

PYBIND_REGISTER(RepeatNode, 2, ([](const py::module *m) {
                  (void)py::class_<RepeatNode, DatasetNode, std::shared_ptr<RepeatNode>>(*m, "RepeatNode",
                                                                                         "to create a RepeatNode")
                    .def(py::init([](std::shared_ptr<DatasetNode> input, int32_t count) {
                      auto repeat = std::make_shared<RepeatNode>(input, count);
                      THROW_IF_ERROR(repeat->ValidateParams());
                      return repeat;
                    }));
                }));

PYBIND_REGISTER(ShuffleNode, 2, ([](const py::module *m) {
                  (void)py::class_<ShuffleNode, DatasetNode, std::shared_ptr<ShuffleNode>>(*m, "ShuffleNode",
                                                                                           "to create a ShuffleNode")
                    .def(py::init([](std::shared_ptr<DatasetNode> self, int32_t shuffle_size, bool reset_every_epoch) {
                      auto shuffle = std::make_shared<ShuffleNode>(self, shuffle_size, reset_every_epoch);
                      THROW_IF_ERROR(shuffle->ValidateParams());
                      return shuffle;
                    }));
                }));

PYBIND_REGISTER(SkipNode, 2, ([](const py::module *m) {
                  (void)py::class_<SkipNode, DatasetNode, std::shared_ptr<SkipNode>>(*m, "SkipNode",
                                                                                     "to create a SkipNode")
                    .def(py::init([](std::shared_ptr<DatasetNode> self, int32_t count) {
                      auto skip = std::make_shared<SkipNode>(self, count);
                      THROW_IF_ERROR(skip->ValidateParams());
                      return skip;
                    }));
                }));

PYBIND_REGISTER(SyncWaitNode, 2, ([](const py::module *m) {
                  (void)py::class_<SyncWaitNode, DatasetNode, std::shared_ptr<SyncWaitNode>>(*m, "SyncWaitNode",
                                                                                             "to create a SyncWaitNode")
                    .def(
                      py::init([](std::shared_ptr<DatasetNode> self, std::string condition_name, py::object callback) {
                        py::function callback_func =
                          py::isinstance<py::function>(callback) ? callback.cast<py::function>() : py::function();
                        auto sync_wait = std::make_shared<SyncWaitNode>(self, condition_name, callback);
                        THROW_IF_ERROR(sync_wait->ValidateParams());
                        return sync_wait;
                      }));
                }));

PYBIND_REGISTER(TakeNode, 2, ([](const py::module *m) {
                  (void)py::class_<TakeNode, DatasetNode, std::shared_ptr<TakeNode>>(*m, "TakeNode",
                                                                                     "to create a TakeNode")
                    .def(py::init([](std::shared_ptr<DatasetNode> self, int32_t count) {
                      auto take = std::make_shared<TakeNode>(self, count);
                      THROW_IF_ERROR(take->ValidateParams());
                      return take;
                    }));
                }));

PYBIND_REGISTER(TransferNode, 2, ([](const py::module *m) {
                  (void)py::class_<TransferNode, DatasetNode, std::shared_ptr<TransferNode>>(*m, "TransferNode",
                                                                                             "to create a TransferNode")
                    .def(py::init([](std::shared_ptr<DatasetNode> self, std::string queue_name, std::string device_type,
                                     int32_t device_id, bool send_epoch_end, int32_t total_batch,
                                     bool create_data_info_queue) {
                      auto transfer = std::make_shared<TransferNode>(
                        self, queue_name, device_type, device_id, send_epoch_end, total_batch, create_data_info_queue);
                      THROW_IF_ERROR(transfer->ValidateParams());
                      return transfer;
                    }));
                }));

PYBIND_REGISTER(ZipNode, 2, ([](const py::module *m) {
                  (void)py::class_<ZipNode, DatasetNode, std::shared_ptr<ZipNode>>(*m, "ZipNode", "to create a ZipNode")
                    .def(py::init([](std::vector<std::shared_ptr<DatasetNode>> datasets) {
                      auto zip = std::make_shared<ZipNode>(datasets);
                      THROW_IF_ERROR(zip->ValidateParams());
                      return zip;
                    }));
                }));

}  // namespace dataset
}  // namespace mindspore
