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

#include "tools/converter/config_parser/config_file_parser.h"
#include "tools/common/parse_config_utils.h"
#include "include/errorcode.h"
#include "src/common/log_adapter.h"
#include "tools/converter/converter_context.h"

#include "tools/common/string_util.h"
#include "src/common/config_infos.h"
#include "src/common/common.h"

namespace mindspore {
namespace lite {
namespace {
constexpr auto kCommonQuantParam = "common_quant_param";
constexpr auto kFullQuantParam = "full_quant_param";
constexpr auto kWeightQuantParam = "weight_quant_param";
constexpr auto kMixedBitWeightQuantParam = "mixed_bit_weight_quant_param";
constexpr auto kDataPreprocessParam = "data_preprocess_param";
constexpr auto kRegistry = "registry";
constexpr auto kAclOptionParam = "acl_option_cfg_param";
constexpr auto kMicroParam = "micro_param";
}  // namespace
using ShapeVector = std::vector<int64_t>;
const int kBatchDim = 0;
const int kDynImgSize = 0;
const int kDynBatchSize = 1;
bool CheckBatchStringSupport(const std::vector<std::string> &batch_str_vec) {
  if (batch_str_vec.empty()) {
    return false;
  }
  std::string only_batch = batch_str_vec[0];
  for (size_t i = 1; i < batch_str_vec.size(); ++i) {
    if (batch_str_vec[i] != only_batch) {
      return false;
    }
  }
  return true;
}

const size_t kIndex0 = 0;
const size_t kIndex1 = 1;
const size_t kIndex2 = 2;
const size_t kIndex3 = 3;
const int64_t kdynDim = -1;
int DynBatchOrDynImage(const ShapeVector &shape) {
  if (shape.size() != 4) {
    MS_LOG(ERROR) << "Do not support input shape which is not equal to 4 (N C H W)";
    return -1;
  }
  if (shape[kIndex0] != -1 && ((shape[kIndex1] == kdynDim && shape[kIndex2] == kdynDim && shape[kIndex3] != kdynDim) ||
                               (shape[kIndex1] == kdynDim && shape[kIndex2] != kdynDim && shape[kIndex3] == kdynDim) ||
                               (shape[kIndex1] != kdynDim && shape[kIndex2] == kdynDim && shape[kIndex3] == kdynDim))) {
    return kDynImgSize;
  }
  if (shape[kIndex0] == kdynDim && shape[kIndex1] != kdynDim && shape[kIndex2] != kdynDim &&
      shape[kIndex3] != kdynDim) {
    return kDynBatchSize;
  }
  return -1;
}

std::string CombineDynImgString(const struct mindspore::ProfileConfigs &profile) {
  ShapeVector shape = profile.input_infos[kIndex0].input_shape;
  std::string ret = "";
  size_t first_dim = kIndex0, second_dim = kIndex0;
  if (shape[kIndex1] == kdynDim && shape[kIndex2] == kdynDim) {
    first_dim = kIndex1;
    second_dim = kIndex2;
  } else if (shape[kIndex1] == kdynDim && shape[kIndex3] == kdynDim) {
    first_dim = kIndex1;
    second_dim = kIndex3;
  } else if (shape[kIndex2] == kdynDim && shape[kIndex3] == kdynDim) {
    first_dim = kIndex2;
    second_dim = kIndex3;
  }
  for (size_t dim_idx = 0; dim_idx < profile.profiles.size(); ++dim_idx) {
    int64_t min_first = profile.profiles[dim_idx].inputs[kIndex0].min_dims[first_dim];
    int64_t max_first = profile.profiles[dim_idx].inputs[kIndex0].max_dims[first_dim];
    int64_t min_second = profile.profiles[dim_idx].inputs[kIndex0].min_dims[second_dim];
    int64_t max_second = profile.profiles[dim_idx].inputs[kIndex0].max_dims[second_dim];
    for (int64_t i = min_first; i <= max_first; ++i) {
      for (int64_t j = min_second; j <= max_second; ++j) {
        ret += std::to_string(i) + "," + std::to_string(j) + ";";
      }
    }
  }
  ret = ret.substr(0, ret.size() - 1);  // discard the final ";"
  return ret;
}

std::string RemoveInputShapeBrackets(const std::string &input_shape_str) {
  std::string ret = "";
  for (size_t i = 0; i < input_shape_str.size(); ++i) {
    if (input_shape_str[i] == '[' || input_shape_str[i] == ']') {
      continue;
    }
    ret += input_shape_str[i];
  }
  return ret;
}

std::string FindInAscendMap(const std::string &key, const std::map<std::string, std::string> &ascend_map) {
  auto it = ascend_map.find(key);
  if (it != ascend_map.end()) {
    return it->second;
  }
  return "";
}

void SetDynParams(const std::shared_ptr<mindspore::ConverterPara> &param,
                  const std::map<std::string, std::string> &ascend_map) {
  struct mindspore::ProfileConfigs tmp_profile;
  if (!mindspore::ProfileParser::Parse(ascend_map, false, &tmp_profile)) {
    MS_LOG(ERROR) << "Parse dynamic_dims failed";
  }
  ShapeVector input_shape_vec = tmp_profile.input_infos[0].input_shape;
  switch (DynBatchOrDynImage(input_shape_vec)) {
    case kDynImgSize:
      param->aclModelOptionCfgParam.dynamic_image_size = CombineDynImgString(tmp_profile);
      break;
    case kDynBatchSize:
      for (size_t i = 0; i < tmp_profile.profiles.size(); ++i) {  // dynamic batch size
        int64_t min_batch = tmp_profile.profiles[i].inputs[0].min_dims[kBatchDim];
        int64_t max_batch = tmp_profile.profiles[i].inputs[0].max_dims[kBatchDim];
        for (int64_t batch = min_batch; batch <= max_batch; ++batch) {
          param->aclModelOptionCfgParam.dynamic_batch_size.push_back(batch);
        }
      }
      break;
    default:
      MS_LOG(ERROR) << "Do not support input shape which is not equal to 4 (N C H W)";
  }
}

void ConfigFileParser::SetParamByConfigfile(const std::shared_ptr<mindspore::ConverterPara> &param,
                                            const std::map<std::string, std::string> &ascend_map) {
  std::string ascend_string = "";
  if (!(ascend_string = FindInAscendMap("input_format", ascend_map)).empty()) {
    param->aclModelOptionCfgParam.input_format = ascend_string;
  }
  if (!(ascend_string = FindInAscendMap("precision_mode", ascend_map)).empty()) {
    param->aclModelOptionCfgParam.precision_mode = ascend_string;
  }
  if (!(ascend_string = FindInAscendMap("op_select_impl_mode", ascend_map)).empty()) {
    param->aclModelOptionCfgParam.op_select_impl_mode = ascend_string;
  }
  if (!(ascend_string = FindInAscendMap("fusion_switch_config_file_path", ascend_map)).empty()) {
    param->aclModelOptionCfgParam.fusion_switch_config_file_path = ascend_string;
  }
  if (!(ascend_string = FindInAscendMap("buffer_optimize", ascend_map)).empty()) {
    param->aclModelOptionCfgParam.buffer_optimize = ascend_string;
  }
  if (!(ascend_string = FindInAscendMap("insert_op_config_file_path", ascend_map)).empty()) {
    param->aclModelOptionCfgParam.insert_op_config_file_path = ascend_string;
  }
  if (!(ascend_string = FindInAscendMap("om_file_path", ascend_map)).empty()) {
    param->aclModelOptionCfgParam.om_file_path = ascend_string;
  }
  if (!(ascend_string = FindInAscendMap("aoe_mode", ascend_map)).empty()) {
    param->aclModelOptionCfgParam.aoe_mode = ascend_string;
  }
  if (!(ascend_string = FindInAscendMap(kDumpModelNameKey, ascend_map)).empty()) {
    param->aclModelOptionCfgParam.dump_model_name = ascend_string;
  }

  auto it = ascend_map.find("input_shape");
  if (it != ascend_map.end()) {
    param->aclModelOptionCfgParam.input_shape = RemoveInputShapeBrackets(it->second);
  }

  it = ascend_map.find("device_id");
  if (it != ascend_map.end()) {
    int32_t val;
    if (mindspore::lite::ConvertIntNum(it->second, &val)) {
      param->aclModelOptionCfgParam.device_id = val;
    } else {
      MS_LOG(ERROR) << "Convert device id failed";
    }
  }

  it = ascend_map.find("output_type");
  if (it != ascend_map.end()) {
    int32_t val;
    if (mindspore::lite::ConvertIntNum(it->second, &val)) {
      param->aclModelOptionCfgParam.output_type = static_cast<mindspore::DataType>(val);
    } else {
      MS_LOG(ERROR) << "Convert output_type failed";
    }
  }

  it = ascend_map.find("dynamic_dims");
  if (it != ascend_map.end()) {
    std::vector<std::string> batch_size_string = mindspore::lite::SplitStringToVector(it->second, ';');
    if (!CheckBatchStringSupport(batch_size_string)) {
      MS_LOG(ERROR) << "Do not support different dynamic_dims!";
      return;
    }
    SetDynParams(param, ascend_map);
  }
}

int ConfigFileParser::ParseConfigFile(const std::string &config_file_path) {
  std::map<std::string, std::map<std::string, std::string>> maps;
  auto ret = mindspore::lite::ParseConfigFile(config_file_path, &maps);
  if (ret != RET_OK) {
    MS_LOG(ERROR) << "Parse config file failed.";
    return ret;
  }
  ret = ParseConfigParam(&maps);
  if (ret != RET_OK) {
    MS_LOG(ERROR) << "Parse config param failed.";
    return ret;
  }
  return RET_OK;
}

int ConfigFileParser::ParseConfigParam(std::map<std::string, std::map<std::string, std::string>> *maps) {
  if (maps == nullptr) {
    MS_LOG(ERROR) << "Maps is nullptr.";
    return RET_ERROR;
  }
  for (const auto &config_info : *maps) {
    ConverterInnerContext::GetInstance()->SetExternalUsedConfigInfos(config_info.first, config_info.second);
  }
  auto ret = ParseDataPreProcessString(*maps);
  (void)maps->erase(kDataPreprocessParam);
  if (ret != RET_OK) {
    MS_LOG(ERROR) << "ParseDataPreProcessString failed.";
    return ret;
  }
  ret = ParseCommonQuantString(*maps);
  (void)maps->erase(kCommonQuantParam);
  if (ret != RET_OK) {
    MS_LOG(ERROR) << "ParseCommonQuantString failed.";
    return ret;
  }
  ret = ParseMixedBitQuantString(*maps);
  (void)maps->erase(kMixedBitWeightQuantParam);
  if (ret != RET_OK) {
    MS_LOG(ERROR) << "ParseMixedBitQuantString failed.";
    return ret;
  }
  ret = ParseFullQuantString(*maps);
  (void)maps->erase(kFullQuantParam);
  if (ret != RET_OK) {
    MS_LOG(ERROR) << "ParseFullQuantString failed.";
    return ret;
  }
  ret = ParseRegistryInfoString(*maps);
  (void)maps->erase(kRegistry);
  if (ret != RET_OK) {
    MS_LOG(ERROR) << "ParseExtendedintegrationString failed.";
    return ret;
  }
  ret = ParseAclOptionCfgString(*maps);
  (void)maps->erase(kAclOptionParam);
  if (ret != RET_OK) {
    MS_LOG(ERROR) << "ParseAclOptionCfgString failed.";
    return ret;
  }
  ret = ParseMicroParamString(*maps);
  (void)maps->erase(kMicroParam);
  if (ret != RET_OK) {
    MS_LOG(ERROR) << "ParseMicroParamString failed.";
    return ret;
  }
  ret = ParseWeightQuantString(*maps);
  (void)maps->erase(kWeightQuantParam);
  if (ret != RET_OK) {
    MS_LOG(ERROR) << "ParseWeightQuantString failed.";
    return ret;
  }
  return RET_OK;
}

int ConfigFileParser::SetMapData(const std::map<std::string, std::string> &input_map,
                                 const std::map<std::string, std::string &> &parse_map, const std::string &section) {
  for (const auto &map : input_map) {
    if (parse_map.find(map.first) == parse_map.end()) {
      MS_LOG(ERROR) << "INPUT ILLEGAL: `" << map.first << "` is not supported in "
                    << "[" << section << "]";
      return RET_INPUT_PARAM_INVALID;
    } else {
      parse_map.at(map.first) = map.second;
    }
  }
  return RET_OK;
}

int ConfigFileParser::ParseDataPreProcessString(const std::map<std::string, std::map<std::string, std::string>> &maps) {
  if (maps.find(kDataPreprocessParam) != maps.end()) {
    const auto &map = maps.at(kDataPreprocessParam);
    std::map<std::string, std::string &> parse_map{
      {"calibrate_path", data_pre_process_string_.calibrate_path},
      {"calibrate_size", data_pre_process_string_.calibrate_size},
      {"input_type", data_pre_process_string_.input_type},
      {"image_to_format", data_pre_process_string_.image_to_format},
      {"normalize_mean", data_pre_process_string_.normalize_mean},
      {"normalize_std", data_pre_process_string_.normalize_std},
      {"resize_width", data_pre_process_string_.resize_width},
      {"resize_height", data_pre_process_string_.resize_height},
      {"resize_method", data_pre_process_string_.resize_method},
      {"center_crop_width", data_pre_process_string_.center_crop_width},
      {"center_crop_height", data_pre_process_string_.center_crop_height},
    };
    return SetMapData(map, parse_map, kDataPreprocessParam);
  }
  return RET_OK;
}

int ConfigFileParser::ParseCommonQuantString(const std::map<std::string, std::map<std::string, std::string>> &maps) {
  if (maps.find(kCommonQuantParam) != maps.end()) {
    const auto &map = maps.at(kCommonQuantParam);
    std::map<std::string, std::string &> parse_map{
      {"quant_type", common_quant_string_.quant_type},
      {"bit_num", common_quant_string_.bit_num},
      {"min_quant_weight_size", common_quant_string_.min_quant_weight_size},
      {"min_quant_weight_channel", common_quant_string_.min_quant_weight_channel},
      {"skip_quant_node", common_quant_string_.skip_quant_node},
      {"debug_info_save_path", common_quant_string_.debug_info_save_path},
      {"enable_encode", common_quant_string_.enable_encode},
    };
    return SetMapData(map, parse_map, kCommonQuantParam);
  }
  return RET_OK;
}

int ConfigFileParser::ParseMixedBitQuantString(const std::map<std::string, std::map<std::string, std::string>> &maps) {
  if (maps.find(kMixedBitWeightQuantParam) != maps.end()) {
    const auto &map = maps.at(kMixedBitWeightQuantParam);
    std::map<std::string, std::string &> parse_map{
      {"init_scale", mixed_bit_quant_string_.init_scale},   {"auto_tune", mixed_bit_quant_string_.auto_tune},
      {"use_cv_data", mixed_bit_quant_string_.use_cv_data}, {"max_iterations", mixed_bit_quant_string_.max_iterations},
      {"workspace", mixed_bit_quant_string_.workspace},
    };
    return SetMapData(map, parse_map, kMixedBitWeightQuantParam);
  }
  return RET_OK;
}

int ConfigFileParser::ParseFullQuantString(const std::map<std::string, std::map<std::string, std::string>> &maps) {
  if (maps.find(kFullQuantParam) != maps.end()) {
    const auto &map = maps.at(kFullQuantParam);
    std::map<std::string, std::string &> parse_map{
      {"activation_quant_method", full_quant_string_.activation_quant_method},
      {"bias_correction", full_quant_string_.bias_correction},
      {"target_device", full_quant_string_.target_device},
      {"per_channel", full_quant_string_.per_channel},
    };
    return SetMapData(map, parse_map, kFullQuantParam);
  }
  return RET_OK;
}

int ConfigFileParser::ParseRegistryInfoString(const std::map<std::string, std::map<std::string, std::string>> &maps) {
  if (maps.find(kRegistry) != maps.end()) {
    const auto &map = maps.at(kRegistry);
    std::map<std::string, std::string &> parse_map{
      {"plugin_path", registry_info_string_.plugin_path},
      {"disable_fusion", registry_info_string_.disable_fusion},
      {"fusion_blacklists", registry_info_string_.fusion_blacklists},
    };
    return SetMapData(map, parse_map, kRegistry);
  }
  return RET_OK;
}

int ConfigFileParser::ParseAclOptionCfgString(const std::map<std::string, std::map<std::string, std::string>> &maps) {
  if (maps.find(kAclOptionParam) != maps.end()) {
    const auto &map = maps.at(kAclOptionParam);
    std::map<std::string, std::string &> parse_map{
      {"device_id", acl_option_cfg_string_.device_id},
      {"input_format", acl_option_cfg_string_.input_format},
      {"input_shape_vector", acl_option_cfg_string_.input_shape_vector},
      {"input_shape", acl_option_cfg_string_.input_shape},
      {"output_type", acl_option_cfg_string_.output_type},
      {"precision_mode", acl_option_cfg_string_.precision_mode},
      {"op_select_impl_mode", acl_option_cfg_string_.op_select_impl_mode},
      {"fusion_switch_config_file_path", acl_option_cfg_string_.fusion_switch_config_file_path},
      {"dynamic_batch_size", acl_option_cfg_string_.dynamic_batch_size},
      {"buffer_optimize", acl_option_cfg_string_.buffer_optimize},
      {"insert_op_config_file_path", acl_option_cfg_string_.insert_op_config_file_path},
      {"dynamic_image_size", acl_option_cfg_string_.dynamic_image_size},
      {"aoe_mode", acl_option_cfg_string_.aoe_mode}};
    return SetMapData(map, parse_map, kAclOptionParam);
  }
  return RET_OK;
}

int ConfigFileParser::ParseMicroParamString(const std::map<std::string, std::map<std::string, std::string>> &maps) {
  if (maps.find(kMicroParam) != maps.end()) {
    const auto &map = maps.at(kMicroParam);
    std::map<std::string, std::string &> parse_map{{"target", micro_param_string_.target},
                                                   {"codegen_mode", micro_param_string_.codegen_mode},
                                                   {"debug_mode", micro_param_string_.debug_mode},
                                                   {"support_parallel", micro_param_string_.support_parallel},
                                                   {"enable_micro", micro_param_string_.enable_micro}};
    return SetMapData(map, parse_map, kMicroParam);
  }
  return RET_OK;
}

int ConfigFileParser::ParseWeightQuantString(const std::map<std::string, std::map<std::string, std::string>> &maps) {
  if (maps.find(kWeightQuantParam) != maps.end()) {
    const auto &map = maps.at(kWeightQuantParam);
    std::map<std::string, std::string &> parse_map{{"dequant_strategy", weight_quant_string_.dequant_strategy},
                                                   {"update_mindir", weight_quant_string_.update_mindir}};
    return SetMapData(map, parse_map, kWeightQuantParam);
  }
  return RET_OK;
}
}  // namespace lite
}  // namespace mindspore
