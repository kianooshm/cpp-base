// Copyright (C) 2012-2013, Middleware Systems Research Group at the
// University of Toronto (www.msrg.org). All rights reserved.

// Licensed under the GNU General Public License Version 3.0. Refer to the
// license file accompanying this program for more information.

#include "cpp-base/file/file.h"

#include <errno.h>
#include <fcntl.h>
#include <limits.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/stat.h>
#include <sys/types.h>
#include <unistd.h>

#include "cpp-base/string/join.h"

using std::string;

namespace cpp_base {

File* File::Open(const char* path, const char* mode, string* error) {
    File* fp = new File(path, mode);
    if (fp->fd_ >= 0)
        return fp;

    if (error != NULL)
        *error = fp->error_;
    delete fp;
    return NULL;
}

bool File::Remove(const char* path, string* error) {
    if (remove(path) != 0) {
        if (error != NULL)
            *error = string(strerror(errno));
        return false;
    }
    return true;
}

int32 File::Size(const char* path, string* error) {
    struct stat st;
    if (stat(path, &st) != 0) {
        if (error != NULL)
            *error = string(strerror(errno));
        else
            LOG(ERROR) << strerror(errno);
        return -1;
    }
    return st.st_size;
}

bool File::Exists(const char* path, string* error) {
    int ret = access(path, F_OK);
    if (ret == 0)
        return true;
    else if (errno == ENOENT)   // File simply doesn't exist
        return false;
    // An error happened:
    if (error != NULL)
        *error = string(strerror(errno));
    else
        LOG(ERROR) << strerror(errno);
    return false;
}

File::File(const char* path, const char* mode) : fd_(-1), error_("") {
    mode_t permissions = S_IRUSR | S_IWUSR | S_IRGRP;
    if (!strcmp(mode, "r")) {
        fd_ = open(path, O_RDONLY);
    } else if (!strcmp(mode, "w")) {
        fd_ = open(path, O_WRONLY | O_CREAT | O_TRUNC, permissions);
    } else if (!strcmp(mode, "rw")) {
        fd_ = open(path, O_RDWR | O_CREAT, permissions);
    } else if (!strcmp(mode, "a")) {
        fd_ = open(path, O_WRONLY | O_APPEND | O_CREAT, permissions);
    } else {
        error_ = StrCat("Invalid mode: ", mode);
        return;
    }
    if (fd_ < 0)
        error_ = string(strerror(errno));
}

File::~File() {
    if (fd_ >= 0)
        close(fd_);
}

bool File::Close() {
    error_.clear();
    if (fd_ < 0) {
        error_ = string("File not open");
        return false;
    }
    if (close(fd_) != 0) {
        error_ = string(strerror(errno));
        return false;
    }
    fd_ = -1;
    return true;
}

string File::LastErrorMsg() {
    return error_;
}

int32 File::Size() {
    struct stat st;
    if (fstat(fd_, &st) != 0) {
        error_ = string(strerror(errno));
        return -1;
    }
    return st.st_size;
}

int32 File::Read(void* buff, int32 size) {
    CHECK_LE(size, SSIZE_MAX);
    error_.clear();
    if (fd_ < 0) {
        error_ = string("File not open");
        return -1;
    }

    int ret = read(fd_, buff, size);
    if (ret < 0)
        error_ = string(strerror(errno));
    return ret;
}

int32 File::Write(const void* buff, int32 size) {
    error_.clear();
    if (fd_ < 0) {
        error_ = string("File not open");
        return -1;
    }

    int ret = write(fd_, buff, size);
    if (ret < 0)
        error_ = string(strerror(errno));
    return ret;
}

}  // namespace cpp_base
