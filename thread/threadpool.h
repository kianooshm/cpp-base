// Copyright 2010-2014 Google
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

#ifndef CPP_BASE_THREAD_THREADPOOL_H_
#define CPP_BASE_THREAD_THREADPOOL_H_

#include <condition_variable>   // NOLINT
#include <mutex>                // NOLINT
#include <list>
#include <string>
#include <thread>               // NOLINT
#include <vector>

#include "cpp-base/callback.h"

namespace cpp_base {

class ThreadPool {
 public:
  explicit ThreadPool(int num_threads);
  ~ThreadPool();

  void StartWorkers();
  void Add(Closure* const closure);
  Closure* GetNextTask();

 private:
  const int num_workers_;
  std::list<Closure*> tasks_;
  std::mutex mutex_;
  std::condition_variable condition_;
  bool waiting_to_finish_;
  bool started_;
  std::vector<std::thread> all_workers_;
};

}  // namespace cpp_base

#endif  // CPP_BASE_THREAD_THREADPOOL_H_
