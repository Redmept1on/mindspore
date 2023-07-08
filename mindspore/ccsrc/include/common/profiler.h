/**
 * Copyright 2023 Huawei Technologies Co., Ltd
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

#ifndef MINDSPORE_CCSRC_RUNTIME_PROFILER_PROFILER_H_
#define MINDSPORE_CCSRC_RUNTIME_PROFILER_PROFILER_H_

#include <vector>
#include <string>
#include <map>
#include <memory>
#include <thread>
#include <mutex>
#include "nlohmann/json.hpp"
#include "utils/os.h"
#include "utils/ms_utils.h"
#include "utils/hash_map.h"
#include "utils/log_adapter.h"
#include "include/common/visible.h"

namespace mindspore {
namespace runtime {
static const char kDefaultOpName[] = "Default";
static const size_t kPercent = 100;

enum class ProfilerStage {
  kDefault,
  kPython,
  kRunGraph,
  kRunGradGraph,
  kRunOp,
  kAsnumpy,
  kCompileGradGraph,
  kWaitPipeline,
  kSyncStream,
};

enum class ProfilerModule { kDefault, kRuntime, kPynative, kKernel, kPython, kOther };

enum class ProfilerEvent {
  kDefault,
  kKernelInfer,
  kKernelResize,
  kKernelLaunch,
  kKernelUpdate,
  kGraphLaunch,
  kInputProcess,
  kOutputProcess,
  kWaitTaskFinish,
  kPreLaunch,
  kPostLaunch,
  kSendOutput,
  kMemoryAlloc,
  kMemoryFree,
  kCopyData,
  kStreamSync,

  // Inner event is not counted in the total time.
  kKernelInferInner,
  kKernelInferDataSync,

  // PyNative Pipeline
  kPyNativeFrontendTask,
  kPyNativeBackendTask,
  kPyNativeDeviceTask,
  kPyNativeBpropTask,
  // PyNative inner Event
  kPyNativeCast,
  kPyNativeInfer,
};

#define PROFILER_START(start_time)                                          \
  do {                                                                      \
    if (runtime::ProfilerAnalyzer::GetInstance().profiler_enable()) {       \
      start_time = runtime::ProfilerAnalyzer::GetInstance().GetTimeStamp(); \
    }                                                                       \
  } while (0);

// Match PROFILER_START to use.
#define PROFILER_END(start_time, module, event, op_name, is_inner_event)                                           \
  do {                                                                                                             \
    if (runtime::ProfilerAnalyzer::GetInstance().profiler_enable()) {                                              \
      auto end_time = runtime::ProfilerAnalyzer::GetInstance().GetTimeStamp();                                     \
      auto brief_name = runtime::ProfilerAnalyzer::GetInstance().GetBriefName(op_name);                            \
      runtime::ProfilerAnalyzer::GetInstance().RecordData(                                                         \
        std::make_shared<runtime::ProfilerData>(module, event, brief_name, is_inner_event, start_time, end_time)); \
    }                                                                                                              \
  } while (0);

// Match PROFILER_START to use.
#define PROFILER_STAGE_END(start_time, stage)                                  \
  do {                                                                         \
    if (runtime::ProfilerAnalyzer::GetInstance().profiler_enable()) {          \
      auto end_time = runtime::ProfilerAnalyzer::GetInstance().GetTimeStamp(); \
      runtime::ProfilerAnalyzer::GetInstance().RecordData(                     \
        std::make_shared<runtime::ProfilerData>(stage, start_time, end_time)); \
    }                                                                          \
  } while (0);

// Record the profiler data by the constructor and destructor of this class.
class COMMON_EXPORT ProfilerRecorder {
 public:
  ProfilerRecorder(ProfilerModule module, ProfilerEvent event, const std::string &op_name, bool is_inner_event = false);
  ~ProfilerRecorder();

 private:
  ProfilerModule module_{ProfilerModule::kDefault};
  ProfilerEvent event_{ProfilerEvent::kDefault};
  std::string op_name_{};
  uint64_t start_time_{0L};
  bool is_inner_event_{false};
};

class COMMON_EXPORT ProfilerStageRecorder {
 public:
  explicit ProfilerStageRecorder(ProfilerStage stage);
  ~ProfilerStageRecorder();

 private:
  ProfilerStage stage_{ProfilerStage::kDefault};
  uint64_t start_time_{0};
};

struct ProfilerData {
  ProfilerData(ProfilerModule module, ProfilerEvent event, const std::string &op_name, bool is_inner_event,
               uint64_t start_time, uint64_t end_time)
      : is_stage_(false),
        stage_(ProfilerStage::kDefault),
        module_(module),
        event_(event),
        op_name_(op_name),
        is_inner_event_(is_inner_event),
        start_time_(start_time),
        end_time_(end_time),
        dur_time_(end_time - start_time),
        tid_(std::this_thread::get_id()),
        pid_(getpid()) {}

  ProfilerData(ProfilerStage stage, uint64_t start_time, uint64_t end_time)
      : is_stage_(true),
        stage_(stage),
        module_(ProfilerModule::kDefault),
        event_(ProfilerEvent::kDefault),
        op_name_(""),
        is_inner_event_(false),
        start_time_(start_time),
        end_time_(end_time) {
    dur_time_ = end_time - start_time;
    tid_ = std::this_thread::get_id();
    pid_ = getpid();
  }

  ProfilerData(const ProfilerData &other)
      : is_stage_(other.is_stage_),
        stage_(other.stage_),
        module_(other.module_),
        event_(other.event_),
        op_name_(other.op_name_),
        is_inner_event_(other.is_inner_event_),
        start_time_(other.start_time_),
        end_time_(other.end_time_),
        dur_time_(other.dur_time_),
        tid_(other.tid_),
        pid_(other.pid_) {}

  ProfilerData &operator=(const ProfilerData &other) {
    if (this == &other) {
      return *this;
    }

    is_stage_ = other.is_stage_;
    stage_ = other.stage_;
    module_ = other.module_;
    event_ = other.event_;
    op_name_ = other.op_name_;
    is_inner_event_ = other.is_inner_event_;
    start_time_ = other.start_time_;
    end_time_ = other.end_time_;
    dur_time_ = other.dur_time_;
    tid_ = other.tid_;
    pid_ = other.pid_;
    return *this;
  }

  bool is_stage_{false};
  ProfilerStage stage_{ProfilerStage::kDefault};
  ProfilerModule module_{ProfilerModule::kDefault};
  ProfilerEvent event_{ProfilerEvent::kDefault};
  std::string op_name_{};
  bool is_inner_event_{false};
  uint64_t start_time_{0L};
  uint64_t end_time_{0L};
  uint64_t dur_time_{0L};
  std::thread::id tid_{};
  int32_t pid_{0};
};
using ProfilerDataPtr = std::shared_ptr<ProfilerData>;

struct ProfilerStatisticsInfo {
  explicit ProfilerStatisticsInfo(const std::string &name, bool is_inner_info = false)
      : name_(name), is_inner_info_(is_inner_info), count_(0), total_time_(0), average_time_(0), percent_(0) {}
  std::string name_;
  bool is_inner_info_;
  size_t count_;
  uint64_t total_time_;
  double average_time_;
  double percent_;

  void AccumulateTime(uint64_t time) {
    total_time_ += time;
    ++count_;
  }
  void Average() {
    MS_EXCEPTION_IF_ZERO("count", count_);
    average_time_ = static_cast<double>(total_time_) / count_;
  }
  void CalculatePercent(uint64_t step_total_time) {
    MS_EXCEPTION_IF_ZERO("step_total_time", step_total_time);
    percent_ = (static_cast<double>(total_time_) / step_total_time) * kPercent;
  }
};
using ProfilerStatisticsInfoPtr = std::shared_ptr<ProfilerStatisticsInfo>;

struct ProfilerEventInfo {
  ProfilerStatisticsInfoPtr event_statistics_info_;
  mindspore::HashMap<std::string, ProfilerStatisticsInfoPtr> op_infos_;
};
using ProfilerEventInfoPtr = std::shared_ptr<ProfilerEventInfo>;

struct ProfilerModuleInfo {
  ProfilerStatisticsInfoPtr module_statistics_info_;
  std::map<ProfilerEvent, ProfilerEventInfoPtr> event_infos_;
};
using ProfilerModuleInfoPtr = std::shared_ptr<ProfilerModuleInfo>;

class COMMON_EXPORT ProfilerAnalyzer {
 public:
  static ProfilerAnalyzer &GetInstance() noexcept;

  // Increase step at the step begin.
  void StartStep();
  // Analyze and output profiler data at the step end.
  void EndStep();

  void Clear();

  // The used by ProfilerRecorder to record data.
  bool profiler_enable() const { return profiler_enable_; }
  void RecordData(const ProfilerDataPtr &data) noexcept;
  uint64_t GetTimeStamp() const noexcept;
  std::string GetBriefName(const std::string &scope_name) const;

 private:
  ProfilerAnalyzer() = default;
  ~ProfilerAnalyzer() = default;
  DISABLE_COPY_AND_ASSIGN(ProfilerAnalyzer);

  void Initialize();

  // Process data.
  void SaveJsonData(const ProfilerDataPtr &data);
  void GenerateSummaryData();
  void AnalyzeSummaryData(const ProfilerDataPtr &data);
  void AnalyzeStageSummaryData(const ProfilerDataPtr &data);
  void AnalyzeModuleSummaryData(const ProfilerDataPtr &data);
  void AnalyzeEventSummaryData(std::map<ProfilerEvent, ProfilerEventInfoPtr> *const event_infos,
                               const ProfilerDataPtr &data);
  void AnalyzeOpSummaryData(mindspore::HashMap<std::string, ProfilerStatisticsInfoPtr> *const op_infos,
                            const ProfilerDataPtr &data);
  void AddPythonSummaryData();

  // Dump data.
  void DumpJsonData() const;
  void DumpDetailData() const;
  void DumpSummaryData() const;
  void DumpStageSummaryData(std::stringstream &string_stream) const;
  void DumpModuleSummaryData(std::stringstream &string_stream) const;
  void DumpEventSummaryData(const std::map<ProfilerEvent, ProfilerEventInfoPtr> &event_infos,
                            std::stringstream &string_stream) const;
  void DumpOpSummaryData(const mindspore::HashMap<std::string, ProfilerStatisticsInfoPtr> &op_infos,
                         std::stringstream &string_stream) const;

  // The relevant members of step.
  size_t step_{0};
  uint64_t step_time_{0};
  uint64_t step_start_time_{0};
  uint64_t module_total_time_{0};
  std::vector<ProfilerDataPtr> data_;
  std::mutex data_mutex_;
  nlohmann::json json_infos_;
  // Summary data that removed non-overlapping ranges from data_.
  std::map<ProfilerModule, std::vector<ProfilerDataPtr>> summary_data_;
  // The data analyzed level is module-->event-->op.
  std::map<ProfilerModule, ProfilerModuleInfoPtr> module_infos_;
  std::map<ProfilerStage, ProfilerStatisticsInfoPtr> stage_infos_;

  // Save file name.
  std::string json_file_name_;
  std::string summary_info_file_name_;
  std::string detail_info_file_name_;

  // The relevant members of init.
  int show_top_num_{0};
  bool profiler_enable_{false};
  bool init_{false};
};
}  // namespace runtime
}  // namespace mindspore

#endif  // MINDSPORE_CCSRC_RUNTIME_PROFILER_PROFILER_H_
