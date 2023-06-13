/**
 * Copyright 2022-2023 Huawei Technologies Co., Ltd
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

#include "extendrt/delegate/ascend_ge/ge_device_context.h"
#include <cxxabi.h>
#include "include/common/utils/scoped_long_running.h"
#include "include/api/context.h"
#include "include/api/status.h"
#include "include/backend/device_type.h"
#include "runtime/device/ms_device_shape_transfer.h"
#include "include/transform/graph_ir/utils.h"
#include "external/ge/ge_api.h"
#include "runtime/config.h"
#include "common/config_infos.h"
#include "common/common.h"

namespace mindspore {
Status GeDeviceContext::Initialize(const std::shared_ptr<Context> &context, const ConfigInfos &config_info) {
  MsContext::GetInstance()->set_backend_policy("ge");
  return InitGe(MsContext::GetInstance(), context, config_info);
}

void GeDeviceContext::Destroy() { (void)FinalizeGe(MsContext::GetInstance()); }

Status GeDeviceContext::InitGe(const std::shared_ptr<MsContext> &inst_context, const std::shared_ptr<Context> &context,
                               const ConfigInfos &config_info) {
  MS_EXCEPTION_IF_NULL(inst_context);
  int32_t is_heterogeneous = 0;
  (void)rtGetIsHeterogenous(&is_heterogeneous);
  inst_context->set_param<bool>(MS_CTX_ENABLE_GE_HETEROGENOUS, is_heterogeneous == 1);
  if (inst_context->get_param<bool>(MS_CTX_IS_PYNATIVE_GE_INIT)) {
    return kSuccess;
  }

  if (static_cast<bool>(inst_context->get_param<uint32_t>(MS_CTX_GE_REF))) {
    inst_context->increase_param<uint32_t>(MS_CTX_GE_REF);
    return kSuccess;
  }

  std::map<std::string, std::string> ge_options;
  GetGeOptions(inst_context, context, &ge_options, config_info);
  {
    // Release GIL before calling into (potentially long-running) C++ code
    mindspore::ScopedLongRunning long_running;
    if (ge::GEInitialize(ge_options) != ge::GRAPH_SUCCESS) {
      MS_LOG(ERROR) << "Initialize GE failed: " << ge::GEGetErrorMsg();
      return kLiteError;
    }
  }
  inst_context->increase_param<uint32_t>(MS_CTX_GE_REF);
  MS_LOG(INFO) << "Init ge successful, ge reference = " << inst_context->get_param<uint32_t>(MS_CTX_GE_REF) << ".";
  return kSuccess;
}

void GeDeviceContext::SetDisableReuseMemoryFlag(std::map<std::string, std::string> *ge_options) const {
  MS_EXCEPTION_IF_NULL(ge_options);
  auto env_disable_reuse_memory = common::GetEnv("DISABLE_REUSE_MEMORY");
  if (!env_disable_reuse_memory.empty()) {
    (*ge_options)["ge.exec.disableReuseMemory"] = env_disable_reuse_memory;
  } else {
    (*ge_options)["ge.exec.disableReuseMemory"] = "0";
    MS_LOG(WARNING) << "DISABLE_REUSE_MEMORY is not set in ENV. Now set to default value 0";
  }
}

void GeDeviceContext::GetGeOptions(const std::shared_ptr<MsContext> &ms_context_ptr,
                                   const std::shared_ptr<Context> &context,
                                   std::map<std::string, std::string> *ge_options, const ConfigInfos &config_info) {
  MS_EXCEPTION_IF_NULL(ms_context_ptr);
  MS_EXCEPTION_IF_NULL(context);
  MS_EXCEPTION_IF_NULL(ge_options);

  (*ge_options)["device_id"] = "0";
  (*ge_options)["rank_table_file"] = "";
  auto env_ddk_version = common::GetEnv("DDK_VERSION");
  if (!env_ddk_version.empty()) {
    (*ge_options)["ge.DDK_version"] = env_ddk_version;
  } else {
    (*ge_options)["ge.DDK_version"] = "1.60.T17.B830";
  }
  (*ge_options)["graphType"] = "1";

  if (ms_context_ptr->get_param<std::string>(MS_CTX_GRAPH_MEMORY_MAX_SIZE) != "0") {
    (*ge_options)["ge.graphMemoryMaxSize"] = ms_context_ptr->get_param<std::string>(MS_CTX_GRAPH_MEMORY_MAX_SIZE);
  }

  if (ms_context_ptr->get_param<std::string>(MS_CTX_VARIABLE_MEMORY_MAX_SIZE) != "0") {
    (*ge_options)["ge.variableMemoryMaxSize"] = ms_context_ptr->get_param<std::string>(MS_CTX_VARIABLE_MEMORY_MAX_SIZE);
  }

  auto env_ge = common::GetEnv("MS_ENABLE_GE");
  auto training = common::GetEnv("MS_GE_TRAIN");
  (*ge_options)["ge.graphRunMode"] = "0";
  if (env_ge == "1" && training == "1") {
    (*ge_options)["ge.graphRunMode"] = "1";
  }
  auto graph_run_mode = common::GetEnv("MS_GE_GRAPH_RUN_MODE");
  if (graph_run_mode == "1") {
    (*ge_options)["ge.graphRunMode"] = "1";
  }

  SetDisableReuseMemoryFlag(ge_options);
  SetHcclOptions(context, ge_options, config_info);

  auto env_job_id = common::GetEnv("JOB_ID");
  if (!env_job_id.empty()) {
    (*ge_options)["ge.exec.jobId"] = env_job_id;
  } else {
    (*ge_options)["ge.exec.jobId"] = "0";
    MS_LOG(WARNING) << "JOB_ID is not set in ENV. Now set to default value 0";
  }

  auto env_fe_flag = common::GetEnv("FE_FLAG");
  if (!env_fe_flag.empty()) {
    (*ge_options)["ge.feFlag"] = env_fe_flag;
    MS_LOG(INFO) << "Use FE, make sure fe lib is set in OPTION_EXEC_EXTERN_PLUGIN_PATH.";
  }

  auto env_aicpu_flag = common::GetEnv("AICPU_FLAG");
  if (!env_aicpu_flag.empty()) {
    (*ge_options)["ge.aicpuFlag"] = env_aicpu_flag;
    MS_LOG(INFO) << "Use AICPU, make sure aicpu lib is set in OPTION_EXEC_EXTERN_PLUGIN_PATH.";
  }

  auto env_op_precision = common::GetEnv("MS_GE_OP_PRECISION");
  if (!env_op_precision.empty()) {
    (*ge_options)["ge.exec.op_precision_mode"] = env_op_precision;
    MS_LOG(INFO) << "Use MS_GE_OP_PRECISION, op precision mode path:" << env_op_precision;
  }

  auto proto_lib_path = common::GetEnv("OPTION_PROTO_LIB_PATH");
  if (!proto_lib_path.empty()) {
    char real_path[PATH_MAX] = {0};
    if (realpath(proto_lib_path.c_str(), real_path)) {
      proto_lib_path = real_path;
      (*ge_options)["ge.opsProtoLibPath"] = proto_lib_path;
    }
  } else {
    MS_LOG(WARNING) << "Set proto lib path failed!";
  }

  if (training == "1") {
    (*ge_options)["ge.exec.precision_mode"] = "allow_fp32_to_fp16";
  } else {
    (*ge_options)["ge.exec.precision_mode"] = "force_fp16";
  }

  // Disable the global variable acc, only enable it while adding training graph in pipeline
  (*ge_options)["ge.exec.variable_acc"] = "0";

  // ge heterogeneous mode
  if (ms_context_ptr->get_param<bool>(MS_CTX_ENABLE_GE_HETEROGENOUS)) {
    (*ge_options)["ge.socVersion"] = "Ascend310P3";
  }
}

void GeDeviceContext::SetHcclOptions(const std::shared_ptr<Context> &context,
                                     std::map<std::string, std::string> *ge_options, const ConfigInfos &config_info) {
  auto device_list = context->MutableDeviceInfo();
  auto ascend_info_iter = std::find_if(
    device_list.begin(), device_list.end(),
    [&](std::shared_ptr<mindspore::DeviceInfoContext> &device_info) { return (device_info->GetProvider() == "ge"); });
  if (ascend_info_iter == device_list.end()) {
    MS_LOG(ERROR) << "AscendDeviceInfo is not set. If using distributed inference, make sure device_id "
                     "and rank_id are set in AscendDeviceInfo";
    return;
  }
  auto device_info = *(ascend_info_iter);
  auto ascend_info = device_info->Cast<mindspore::AscendDeviceInfo>();
  std::string rank_table_file = "";
  uint32_t device_id = ascend_info->GetDeviceID();
  uint32_t rank_id = ascend_info->GetRankID();

  if (config_info.empty() || config_info.find(lite::kAscendContextSection) == config_info.end()) {
    MS_LOG(INFO) << "There is no ascend context info in config file.";
  } else {
    auto config_info_ascend = config_info.at(lite::kAscendContextSection);
    if (config_info_ascend.find(lite::kRankTableFilePathKey) == config_info_ascend.end()) {
      MS_LOG(INFO)
        << "There is no rank table file in Ascend section of config file, distributed inference is not enabled."
        << " If using distributed inference, make sure rank_table_file in the config file,"
        << " device_id and rank_id are set in AscendDeviceInfo.";
    } else {
      rank_table_file = config_info_ascend[lite::kRankTableFilePathKey];
      MS_LOG(INFO) << "Distributed inference is enabled, rank table file: " << rank_table_file;
    }
  }

  auto env_cluster_info = common::GetEnv("HELP_CLUSTER");
  MS_LOG(INFO) << "Set ge_options for rank table file " << rank_table_file << " device id " << device_id << " rank id "
               << rank_id;
  if (!(rank_table_file.empty() || !(env_cluster_info.empty()))) {
    MS_LOG(INFO) << "Initialize Ge for distribute parameter";
    if (!rank_table_file.empty()) {
      MS_LOG(INFO) << "Use hccl, make sure hccl lib is set in OPTION_EXEC_EXTERN_PLUGIN_PATH.";
      (*ge_options)["ge.exec.rankTableFile"] = rank_table_file;
    }
    auto env_hccl_flag = common::GetEnv("HCCL_FLAG");
    if (!env_hccl_flag.empty()) {
      (*ge_options)["ge.exec.hcclFlag"] = env_hccl_flag;
    }
    (*ge_options)["ge.exec.isUseHcom"] = "1";
    (*ge_options)["ge.exec.deviceId"] = std::to_string(device_id);
    (*ge_options)["ge.exec.rankId"] = std::to_string(rank_id);
    (*ge_options)["ge.exec.podName"] = std::to_string(rank_id);
  } else {
    // device id is still needed for non-distribute case
    (*ge_options)["ge.exec.deviceId"] = std::to_string(device_id);
    MS_LOG(INFO) << "No hccl mode. "
                 << "If use hccl, make sure that the rank table file path is set in config file, "
                 << "rank id and device id are set in AscendDeviceInfo.";
  }

  auto env_deploy_mode = common::GetEnv("DEPLOY_MODE");
  if (!env_deploy_mode.empty()) {
    (*ge_options)["ge.exec.deployMode"] = env_deploy_mode;
  } else {
    (*ge_options)["ge.exec.deployMode"] = "0";
    MS_LOG(WARNING) << "DEPLOY_MODE is not set in ENV. Now set to default value 0";
  }
}

bool GeDeviceContext::FinalizeGe(const std::shared_ptr<MsContext> &inst_context) {
  MS_EXCEPTION_IF_NULL(inst_context);
  if (inst_context->get_param<uint32_t>(MS_CTX_GE_REF) == 0) {
    return true;
  }
  inst_context->decrease_param<uint32_t>(MS_CTX_GE_REF);
  if (inst_context->get_param<uint32_t>(MS_CTX_GE_REF) == 0) {
    inst_context->set_param<uint32_t>(MS_CTX_GE_REF, 0);
    try {
      transform::ClearGeSessionAndRunner();
    } catch (const std::exception &e) {
      MS_LOG(ERROR) << "Error occurred when deleting GE graph runner and session fail. Error: " << e.what();
    } catch (...) {
      std::string exName(abi::__cxa_current_exception_type()->name());
      MS_LOG(ERROR) << "Error occurred when deleting GE graph runner and session fail. Exception name: " << exName;
    }
    if (ge::GEFinalize() != ge::GRAPH_SUCCESS) {
      MS_LOG(WARNING) << "Finalize GE failed!";
    }
    inst_context->set_param<bool>(MS_CTX_IS_PYNATIVE_GE_INIT, false);
  } else {
    MS_LOG(INFO) << "Ge is used, no need to finalize, tsd reference = "
                 << inst_context->get_param<uint32_t>(MS_CTX_GE_REF) << ".";
  }
  return true;
}
}  // namespace mindspore
