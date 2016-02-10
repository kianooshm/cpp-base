// Copyright (C) 2012-2013, Middleware Systems Research Group at the
// University of Toronto (www.msrg.org). All rights reserved.

// Licensed under the GNU General Public License Version 3.0. Refer to the
// license file accompanying this program for more information.

#ifndef CPP_BASE_FILE_FILE_INPUT_STREAM_H_
#define CPP_BASE_FILE_FILE_INPUT_STREAM_H_

#include <stdio.h>
#include <string>

#include "cpp-base/file/file.h"
#include "cpp-base/integral_types.h"
#include "cpp-base/scoped_ptr.h"

namespace cpp_base {

// Buffered reader from file.
class FileInputStream {
  public:
    // 'error' can be NULL in the following functions.
    static FileInputStream* Open(const char* path, std::string* error = NULL);
    static FileInputStream* Open(const char* path, int buff_size,
                                 std::string* error = NULL);
    static FileInputStream* OpenOrDie(const char* path);

    ~FileInputStream();
    bool Close();
    std::string LastErrorMsg();

    // Note: this flag is set *after* attempting to read from an EOF-ed stream.
    bool ReachedEof();

    // Returns num bytes read; 0 on EOF, -1 on error.
    int32 Read(void* buff, int32 size);

    // Read a string from the current position in the file until the next '\n'
    // or EOF. Removes the possible '\r' at the end of the string. Empty line is
    // ok (e.g. "...\n\n") but reading from an already-EOFed stream will return
    // false.
    bool ReadLine(std::string* line);

    // Returns false on error or EOF.
    template <class T>
    bool ReadGeneric(T* value) {
        if (buff_data_len_ - buff_ptr_ < sizeof(T)) {  // Need to refill buffer
            if (!RefillBuffer() || ReachedEof() ||
                buff_data_len_ - buff_ptr_ < sizeof(T))
                return false;
        }
        CHECK_GE(buff_data_len_ - buff_ptr_, (int32) sizeof(T))
               << buff_data_len_ << ", " << buff_ptr_;
        *value = *(reinterpret_cast<T*>(buffer_ + buff_ptr_));
        buff_ptr_ += sizeof(T);
        return true;
    }

    template <class T>
    void ReadGenericOrDie(T* value) {
        CHECK(ReadGeneric(value)) << LastErrorMsg();
    }

  private:
    FileInputStream(const char* path, int buff_size);

    // Returns true on success (including EOF), false on error.
    bool RefillBuffer();

    scoped_ptr<File> fp_;
    std::string error_;

    char* buffer_;
    int32 buff_size_;
    static const int DEFAULT_BUFF_SIZE = 10 * 1024 * 1024;  // 10 MB

    // buff_ptr_ <= buff_data_len_ <= buff_size_.
    int32 buff_ptr_;
    int32 buff_data_len_;
    bool reached_eof_;
};

}  // namespace cpp_base

#endif  // CPP_BASE_FILE_FILE_INPUT_STREAM_H_
