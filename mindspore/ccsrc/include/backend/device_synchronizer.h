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

#ifndef MINDSPORE_CCSRC_INCLUDE_DEVICE_SYNCHRONIZER_H
#define MINDSPORE_CCSRC_INCLUDE_DEVICE_SYNCHRONIZER_H

#include <cstddef>
#include "include/backend/visible.h"

namespace mindspore {
namespace device {
// This class provides interfaces for synchronizing data between device and host.
class BACKEND_EXPORT DeviceSynchronizer {
 public:
  DeviceSynchronizer() = default;
  virtual ~DeviceSynchronizer() = default;

  // Copy device memory to host side synchronously.
  virtual bool SyncDeviceToHost(void *host_ptr, void *device_ptr, size_t size, size_t stream_id) const = 0;

  // Copy host memory to device side synchronously.
  virtual bool SyncHostToDevice(void *device_ptr, void *host_ptr, size_t size, size_t stream_id) const = 0;
};
}  // namespace device
}  // namespace mindspore
#endif  // MINDSPORE_CCSRC_INCLUDE_DEVICE_SYNCHRONIZER_H
