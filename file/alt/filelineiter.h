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

// Allows to read a text file line by line with:
//   for (const std::string& line : FileLines("myfile.txt")) { ... }
//
// More details:
// * The lines are separated by '\n' (which is removed) and have no size limits.
// * Consecutive '\n' result in empty lines being produced.
// * If not empty, the std::string after the last '\n' is produced as the last line.
//
#ifndef CPP_BASE_FILE_ALT_FILELINEITER_H_
#define CPP_BASE_FILE_ALT_FILELINEITER_H_

#include <glog/logging.h>

#include "cpp-base/file/alt/file.h"
#include "cpp-base/macros.h"

namespace alt_file {

// Implements the minimum interface for a range-based for loop iterator.
class FileLineIterator {
 public:
  explicit FileLineIterator(File* file)
      : next_position_after_eol_(0), buffer_size_(0), file_(file) {
    ReadNextLine();
  }
  const std::string& operator*() const { return line_; }
  bool operator!=(const FileLineIterator& other) const {
    return file_ != other.file_;
  }
  void operator++() { ReadNextLine(); }

 private:
  void ReadNextLine() {
    line_.clear();
    if (file_ == nullptr) return;
    while (1) {
      int i = next_position_after_eol_;
      for (; i < buffer_size_; ++i) {
        if (buffer_[i] == '\n') break;
      }
      line_.append(&buffer_[next_position_after_eol_],
                   i - next_position_after_eol_);
      if (i == buffer_size_) {
        buffer_size_ = file_->Read(&buffer_, kBufferSize);
        if (buffer_size_ < 0) {
          LOG(WARNING) << "Error while reading file.";
          file_ = nullptr;
          break;
        }
        next_position_after_eol_ = 0;
        if (buffer_size_ == 0) {
          if (line_.empty()) {
            file_ = nullptr;
          }
          break;
        }
      } else {
        next_position_after_eol_ = i + 1;
        break;
      }
    }
  }

  static const int kBufferSize = 5 * 1024;
  char buffer_[kBufferSize];
  int next_position_after_eol_;
  int buffer_size_;
  File* file_;
  std::string line_;
};

class FileLines {
 public:
  explicit FileLines(const std::string& filename) {
    file_ = File::Open(filename, "r");
  }
  ~FileLines() {
    if (file_ != nullptr) file_->Close();
  }
  FileLineIterator begin() { return FileLineIterator(file_); }
  FileLineIterator end() const { return FileLineIterator(nullptr); }

 private:
  File* file_;
  DISALLOW_COPY_AND_ASSIGN(FileLines);
};

}  // namespace alt_file

#endif  // CPP_BASE_FILE_ALT_FILELINEITER_H_
