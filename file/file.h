// Copyright (C) 2012-2013, Middleware Systems Research Group at the
// University of Toronto (www.msrg.org). All rights reserved.

// Licensed under the GNU General Public License Version 3.0. Refer to the
// license file accompanying this program for more information.

#ifndef CPP_BASE_FILE_FILE_H_
#define CPP_BASE_FILE_FILE_H_

#include <glog/logging.h>
#include <stdio.h>
#include <string>
#include "cpp-base/integral_types.h"

namespace cpp_base {

class File {
  public:
    // 'error' can be NULL in all the following functions.
    // Open() returns NULL on error.
    static File* Open(const char* path, const char* mode, std::string* error = NULL);
    static bool Remove(const char* path, std::string* error = NULL);
    static int32 Size(const char* path, std::string* error = NULL);
    static bool Exists(const char* path, std::string* error = NULL);

    ~File();
    bool Close();
    std::string LastErrorMsg();
    int32 Size();

    // Returns num bytes read; 0 on EOF, -1 on error.
    int32 Read(void* buff, int32 size);

    // Returns num bytes written (can be 0); -1 on error.
    int32 Write(const void* buff, int32 size);

  private:
    File(const char* path, const char* mode);

    int fd_;
    std::string error_;
};

}  // namespace cpp_base

#endif  // CPP_BASE_FILE_FILE_H_
