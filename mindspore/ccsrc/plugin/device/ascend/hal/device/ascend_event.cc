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

#include "plugin/device/ascend/hal/device/ascend_event.h"

#include "acl/acl.h"
#include "acl/acl_rt.h"
#include "utils/log_adapter.h"

namespace mindspore::device::ascend {
AscendEvent::AscendEvent() {
  auto ret = aclrtCreateEvent(&event_);
  if (ret != ACL_ERROR_NONE) {
    MS_LOG(ERROR) << "aclrtCreateEvent failed, ret:" << ret;
    event_ = nullptr;
  }
}

AscendEvent::AscendEvent(uint32_t flag) {
  auto ret = aclrtCreateEventWithFlag(&event_, flag);
  if (ret != ACL_ERROR_NONE) {
    MS_LOG(ERROR) << "aclrtCreateEventWithFlag failed, ret:" << ret;
    event_ = nullptr;
  }
}

AscendTimeEvent::AscendTimeEvent() {
  auto ret = aclrtCreateEventWithFlag(&event_, ACL_EVENT_TIME_LINE);
  if (ret != ACL_ERROR_NONE) {
    MS_LOG(ERROR) << "aclrtCreateEvent failed, ret:" << ret;
    event_ = nullptr;
  }
}

AscendEvent::~AscendEvent() {
  auto ret = aclrtDestroyEvent(event_);
  if (ret != ACL_ERROR_NONE) {
    MS_LOG(ERROR) << "aclrtDestroyEvent failed, ret:" << ret;
  }

  event_ = nullptr;
  wait_stream_ = nullptr;
  record_stream_ = nullptr;
}

void AscendEvent::RecordEvent() {
  MS_EXCEPTION_IF_NULL(event_);
  MS_EXCEPTION_IF_NULL(record_stream_);
  auto ret = aclrtRecordEvent(event_, record_stream_);
  if (ret != ACL_ERROR_NONE) {
    MS_LOG(EXCEPTION) << "aclrtRecordEvent failed, ret:" << ret;
  }
  need_wait_ = true;
}

void AscendEvent::WaitEvent() {
  MS_EXCEPTION_IF_NULL(event_);
  MS_EXCEPTION_IF_NULL(wait_stream_);
  auto ret = aclrtStreamWaitEvent(wait_stream_, event_);
  if (ret != ACL_ERROR_NONE) {
    MS_LOG(EXCEPTION) << "aclrtStreamWaitEvent failed, ret:" << ret;
  }
  ret = aclrtResetEvent(event_, wait_stream_);
  if (ret != ACL_ERROR_NONE) {
    MS_LOG(EXCEPTION) << "aclrtResetEvent failed, ret:" << ret;
  }
  need_wait_ = false;
}

void AscendEvent::SyncEvent() {
  MS_EXCEPTION_IF_NULL(event_);
  auto ret = aclrtSynchronizeEvent(event_);
  if (ret != ACL_ERROR_NONE) {
    MS_LOG(EXCEPTION) << "aclrtSynchronizeEvent failed, ret:" << ret;
  }
}

bool AscendEvent::QueryEvent() {
  MS_EXCEPTION_IF_NULL(event_);
  aclrtEventRecordedStatus status;
  auto ret = aclrtQueryEventStatus(event_, &status);
  if (ret != ACL_ERROR_NONE) {
    MS_LOG(EXCEPTION) << "aclQueryEventStatus failed, ret:" << ret;
  }
  return status == ACL_EVENT_RECORDED_STATUS_COMPLETE;
}

void AscendEvent::ElapsedTime(float *cost_time, const DeviceEvent *other) {
  MS_EXCEPTION_IF_NULL(event_);
  auto ascend_other = static_cast<const AscendEvent *>(other);
  MS_EXCEPTION_IF_NULL(ascend_other);
  MS_EXCEPTION_IF_NULL(ascend_other->event_);
  auto ret = aclrtEventElapsedTime(cost_time, event_, ascend_other->event_);
  if (ret != ACL_ERROR_NONE) {
    MS_LOG(EXCEPTION) << "aclrtEventElapsedTime failed, ret:" << ret;
  }
}

bool AscendEvent::NeedWait() { return need_wait_; }
}  // namespace mindspore::device::ascend
