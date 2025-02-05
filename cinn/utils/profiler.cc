// Copyright (c) 2022 CINN Authors. All Rights Reserved.
//
// Licensed under the Apache License, Version 2.0 (the "License");
// you may not use this file except in compliance with the License.
// You may obtain a copy of the License at
//
//     http://www.apache.org/licenses/LICENSE-2.0
//
// Unless required by applicable law or agreed to in writing, software
// distributed under the License is distributed on an "AS IS" BASIS,
// WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
// See the License for the specific language governing permissions and
// limitations under the License.

#include "cinn/utils/profiler.h"

#ifdef CINN_WITH_NVTX
#include <nvToolsExt.h>
#endif
#ifdef CINN_WITH_CUDA
#include <cuda_profiler_api.h>
#include <cuda_runtime.h>

#include "cinn/backends/cuda_util.h"
#endif

#include <chrono>

namespace cinn {
namespace utils {

ProfilerState ProfilerHelper::g_state = ProfilerState::kDisabled;

RecordEvent::RecordEvent(const std::string& name, EventType type) {
  if (!ProfilerHelper::IsEnable()) return;

  if (ProfilerHelper::IsEnableCPU()) {
    call_back_ = [this, tik = std::chrono::steady_clock::now(), annotation = std::move(name), type]() {
      auto tok                               = std::chrono::steady_clock::now();
      std::chrono::duration<double> duration = (tok - tik) * 1e3;  // ms
      HostEventRecorder::GetInstance().RecordEvent(annotation, duration.count(), type);
    };
  }

  if (ProfilerHelper::IsEnableCUDA()) {
    ProfilerRangePush(name);
  }
}

void RecordEvent::End() {
  if (!ProfilerHelper::IsEnable()) return;

  if (ProfilerHelper::IsEnableCPU() && call_back_ != nullptr) {
    call_back_();
  }

  if (ProfilerHelper::IsEnableCUDA()) {
    ProfilerRangePop();
  }
}

void SynchronizeAllDevice() {
#ifdef CINN_WITH_CUDA
  int current_device_id;
  CUDA_CALL(cudaGetDevice(&current_device_id));
  int count;
  CUDA_CALL(cudaGetDeviceCount(&count));
  for (int i = 0; i < count; i++) {
    CUDA_CALL(cudaSetDevice(i));
    CUDA_CALL(cudaDeviceSynchronize());
  }
  CUDA_CALL(cudaSetDevice(current_device_id));
#endif
}

void ProfilerStart() {
#ifdef CINN_WITH_CUDA
  CUDA_CALL(cudaProfilerStart());
  SynchronizeAllDevice();
#endif
}

void ProfilerStop() {
#ifdef CINN_WITH_CUDA
  CUDA_CALL(cudaProfilerStop());
#endif
}

void ProfilerRangePush(const std::string& name) {
#ifdef CINN_WITH_NVTX
  nvtxRangePushA(name.c_str());
#endif
}

void ProfilerRangePop() {
#ifdef CINN_WITH_NVTX
  nvtxRangePop();
#endif
}

}  // namespace utils
}  // namespace cinn
