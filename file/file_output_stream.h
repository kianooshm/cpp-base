// Copyright (C) 2012-2013, Middleware Systems Research Group at the
// University of Toronto (www.msrg.org). All rights reserved.

// Licensed under the GNU General Public License Version 3.0. Refer to the
// license file accompanying this program for more information.

#ifndef CPP_BASE_FILE_FILE_OUTPUT_STREAM_H_
#define CPP_BASE_FILE_FILE_OUTPUT_STREAM_H_

#include <stdio.h>
#include <string>

#include "cpp-base/file/file.h"
#include "cpp-base/integral_types.h"
#include "cpp-base/scoped_ptr.h"

namespace cpp_base {

// Buffered writer to file.
class FileOutputStream {
  public:
    // 'error' can be NULL in the following functions.
    static FileOutputStream* Open(const char* path, std::string* error = NULL);
    static FileOutputStream* Open(const char* path, int buff_size,
                                  std::string* error = NULL);
    static FileOutputStream* OpenForAppend(const char* path,
                                           std::string* error = NULL);

    static FileOutputStream* OpenOrDie(const char* path);
    static FileOutputStream* OpenForAppendOrDie(const char* path);

    ~FileOutputStream();
    bool Close();
    bool Flush();
    std::string LastErrorMsg();

    bool Write(const void* buff, int32 size);
    bool Write(const std::string& str);

    void WriteOrDie(const void* buff, int32 size);
    void WriteOrDie(const std::string& str);

    template <class T>
    bool WriteGeneric(const T& value) {
        return Write(&value, sizeof(T));
    }

    template <class T>
    void WriteGenericOrDie(const T& value) {
        CHECK(WriteGeneric(value)) << LastErrorMsg();
    }

  private:
    static FileOutputStream* __open(const char* path, int buff_size,
                                    bool to_append, std::string* error = NULL);
    FileOutputStream(const char* path, int buff_size, bool to_append);

    scoped_ptr<File> fp_;
    std::string error_;
    char* buffer_;
    int32 buff_size_;
    int32 buff_data_len_;
    static const int DEFAULT_BUFF_SIZE = 10 * 1024 * 1024;  // 10 MB
};

}  // namespace cpp_base

#endif  // CPP_BASE_FILE_FILE_OUTPUT_STREAM_H_
