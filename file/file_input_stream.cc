// Copyright (C) 2012-2013, Middleware Systems Research Group at the
// University of Toronto (www.msrg.org). All rights reserved.

// Licensed under the GNU General Public License Version 3.0. Refer to the
// license file accompanying this program for more information.

#include "cpp-base/file/file_input_stream.h"

#include <glog/logging.h>
#include <algorithm>
#include <cstring>

using std::string;

namespace cpp_base {

FileInputStream* FileInputStream::Open(const char* path, string* error) {
    return Open(path, DEFAULT_BUFF_SIZE, error);
}

FileInputStream* FileInputStream::OpenOrDie(const char* path) {
    string error;
    FileInputStream* in = Open(path, &error);
    CHECK(in != NULL) << error << "; path = " << path;
    return in;
}

FileInputStream* FileInputStream::Open(
        const char* path, int buff_size, string* error) {
    FileInputStream* fis = new FileInputStream(path, buff_size);
    if (fis->fp_ != NULL)
        return fis;

    if (error != NULL)
        *error = fis->error_;
    else
        LOG(ERROR) << fis->error_;
    delete fis;
    return NULL;
}

FileInputStream::FileInputStream(const char* path, int buff_size)
        : buff_size_(buff_size), buff_ptr_(0),
          buff_data_len_(0), reached_eof_(false) {
    CHECK_GT(buff_size, 0);
    buffer_ = new char[buff_size];
    CHECK(buffer_ != NULL);
    fp_.reset(File::Open(path, "r", &error_));  // May be NULL
}

FileInputStream::~FileInputStream() {
    delete buffer_;
    // Don't need to call fp_->Close(); it's automatically closed on destruction
}

bool FileInputStream::Close() {
    bool ret = fp_->Close();
    fp_.reset(NULL);
    return ret;
}

string FileInputStream::LastErrorMsg() {
    if (ReachedEof())
        return string("Reached EOF");
    if (fp_ == NULL)
        return string("Invalid/non-opened file handler");
    return error_;
}

bool FileInputStream::RefillBuffer() {
    if (reached_eof_) {
        return true;
    }
    if (buff_ptr_ < buff_data_len_) {
        // Unread data in buffer. This should happen only for a small number
        // of bytes, e.g., trying to read an int64 when buffer pointer is just
        // 2 bytes before the end. Copy the remained data to the beginning of
        // the buffer, and reload the rest.
        int32 len = buff_data_len_ - buff_ptr_;
        CHECK_LE(len, buff_ptr_);
        memcpy(buffer_, buffer_ + buff_ptr_, len);
        buff_ptr_ = 0;
        buff_data_len_ = len;
    } else {
        buff_ptr_ = 0;
        buff_data_len_ = 0;
    }

    int ret = fp_->Read(buffer_ + buff_data_len_, buff_size_ - buff_data_len_);

    if (ret < 0) {
        error_ = fp_->LastErrorMsg();
        return false;
    } else if (ret == 0) {  // EOF
        if (buff_data_len_ == 0)
            reached_eof_ = true;
        return true;
    } else {
        buff_data_len_ += ret;
        return true;
    }
}

bool FileInputStream::ReachedEof() {
    return reached_eof_;
}

int32 FileInputStream::Read(void* buff, int32 size) {
    // Enough data already read into the buffer?
    if (buff_data_len_ - buff_ptr_ >= size) {
        memcpy(buff, buffer_ + buff_ptr_, size);
        buff_ptr_ += size;
        return size;
    }

    int32 bytes_read = 0;
    while (size > 0) {
        bool more_data_available = true;

        CHECK_LE(buff_ptr_, buff_data_len_);
        if (buff_ptr_ == buff_data_len_) {  // Try reading a whole block of data
            if (!RefillBuffer())    // Error (not EOF)
                return -1;
            else if (reached_eof_)  // EOF
                return bytes_read;
            else if (buff_data_len_ < buff_size_)
                more_data_available = false;
        }
        CHECK_LT(buff_ptr_, buff_data_len_);

        int32 read_len = std::min(size, buff_data_len_ - buff_ptr_);
        memcpy(buff, buffer_ + buff_ptr_, read_len);
        buff_ptr_ += read_len;
        size -= read_len;
        buff = reinterpret_cast<char*>(buff) + read_len;
        bytes_read += read_len;

        if (!more_data_available)
            break;
    }

    return bytes_read;
}

bool FileInputStream::ReadLine(string* line) {
    if (ReachedEof())
        return false;

    *line = "";
    while (1) {
        // Look for '\n' in the buffer:
        int32 line_len = -1;
        for (int32 i = buff_ptr_; i < buff_data_len_; ++i) {
            if (buffer_[i] == '\n') {
                line_len = i - buff_ptr_;
                break;
            }
        }
        if (line_len >= 0) {  // Found '\n' in the buffer
            *line += string(buffer_ + buff_ptr_, line_len);
            buff_ptr_ += line_len + 1;
            break;
        }

        // No '\n'. Keep whatever read so far, and reload the buffer:
        int part1_len = buff_data_len_ - buff_ptr_;
        *line += string(buffer_ + buff_ptr_, part1_len);
        buff_ptr_ += part1_len;

        if (!RefillBuffer()) {
            return false;
        }
        if (ReachedEof()) {
            if (line->empty())
                return false;
            else
                break;
        }
        CHECK_LT(buff_ptr_, buff_data_len_);
    }

    // Remove '\r' if the line ended with "\r\n" ('\n' is already excluded).
    if (line->length() > 0 && line->at(line->length() - 1) == '\r')
        line->resize(line->length() - 1);
    return true;
}

}  // namespace cpp_base
