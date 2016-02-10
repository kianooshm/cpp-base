// Copyright (C) 2012-2013, Middleware Systems Research Group at the
// University of Toronto (www.msrg.org). All rights reserved.

// Licensed under the GNU General Public License Version 3.0. Refer to the
// license file accompanying this program for more information.

#include "cpp-base/file/file_output_stream.h"

#include <glog/logging.h>
#include <algorithm>
#include <cstring>

using std::string;

namespace cpp_base {

FileOutputStream* FileOutputStream::Open(const char* path, string* error) {
    return __open(path, DEFAULT_BUFF_SIZE, false, error);
}
FileOutputStream* FileOutputStream::Open(const char* path, int buff_size,
                                         string* error) {
    return __open(path, buff_size, false, error);
}
FileOutputStream* FileOutputStream::OpenForAppend(const char* path,
                                                  string* error) {
    return __open(path, DEFAULT_BUFF_SIZE, true, error);
}
FileOutputStream* FileOutputStream::OpenOrDie(const char* path) {
    string error;
    FileOutputStream* out = __open(path, DEFAULT_BUFF_SIZE, false, &error);
    CHECK(out != NULL) << error;
    return out;
}
FileOutputStream* FileOutputStream::OpenForAppendOrDie(const char* path) {
    string error;
    FileOutputStream* out = __open(path, DEFAULT_BUFF_SIZE, true, &error);
    CHECK(out != NULL) << error;
    return out;
}

FileOutputStream* FileOutputStream::__open(
        const char* path, int buff_size, bool to_append, string* error) {
    FileOutputStream* fos = new FileOutputStream(path, buff_size, to_append);
    if (fos->fp_ != NULL)
        return fos;
    if (error != NULL)
        *error = fos->error_;
    else
        LOG(ERROR) << fos->error_;
    delete fos;
    return NULL;
}

FileOutputStream::FileOutputStream(
        const char* path, int buff_size, bool to_append)
        : buff_size_(buff_size), buff_data_len_(0) {
    CHECK_GT(buff_size, 0);
    buffer_ = new char[buff_size];
    CHECK(buffer_ != NULL);
    fp_.reset(File::Open(path, to_append ? "a" : "w", &error_));  // May be NULL
}

FileOutputStream::~FileOutputStream() {
    if (fp_.get() != NULL)
        Close();  // Close() gets all not-yet-written data flushed.
    delete buffer_;
}

bool FileOutputStream::Close() {
    bool ret1 = Flush();
    bool ret2 = fp_->Close();
    fp_.reset(NULL);
    return ret1 && ret2;
}

bool FileOutputStream::Flush() {
    int32 begin = 0;
    int ret = fp_->Write(buffer_, buff_data_len_);

    while (ret < buff_data_len_ - begin) {
        if (ret < 0) {
            error_ = fp_->LastErrorMsg();
            return false;
        }
        begin += ret;
        ret = fp_->Write(buffer_ + begin, buff_data_len_ - begin);
    }

    buff_data_len_ = 0;
    return true;
}

string FileOutputStream::LastErrorMsg() {
    if (fp_ == NULL)
        return string("Invalid/non-opened file handler");
    return error_;
}

bool FileOutputStream::Write(const void* buff, int32 size) {
    CHECK_GE(size, 0);
    if (size == 0)
        return true;
    if (size <= buff_size_ - buff_data_len_) {
        memcpy(buffer_ + buff_data_len_, buff, size);
        buff_data_len_ += size;
        return true;
    }

    while (size > 0) {
        if (!Flush())
            return false;
        CHECK_EQ(buff_data_len_, 0);

        int32 copy_size = std::min(size, buff_size_);
        memcpy(buffer_, buff, copy_size);
        buff_data_len_ += copy_size;
        buff = reinterpret_cast<const char*>(buff) + copy_size;
        size -= copy_size;
    }

    return true;
}

bool FileOutputStream::Write(const string& str) {
    return Write(str.c_str(), str.length());
}

void FileOutputStream::WriteOrDie(const void* buff, int32 size) {
    CHECK(Write(buff, size)) << LastErrorMsg();
}

void FileOutputStream::WriteOrDie(const string& str) {
    CHECK(Write(str)) << LastErrorMsg();
}

}  // namespace cpp_base
