#include "net/ByteData.h"

#include "net/sockets/SocketsOps.h"
#include <sys/uio.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <assert.h>

namespace Miren {
namespace net {

void ByteData::addDataZeroCopy(const base::StringPiece& data) {
    addDataZeroCopy(data.data(), data.size());
}

void ByteData::addDataZeroCopy(const void *data, size_t size) {
    DataPacket* dp = new DataPacket();
    dp->copy_ = false;
    dp->zero_copy_data_ = (char*)data;
    dp->size_ += size;
    datas_.push_back(dp);
}

void ByteData::addDataCopy(const base::StringPiece &data) {
    addDataCopy(data.data(), data.size());
}

void ByteData::addDataCopy(const void *data, size_t size) {
    DataPacket* dp = new DataPacket();
    dp->copy_ = true;
    dp->copy_data_ = new net::Buffer(size * 2);
    dp->copy_data_->append((const char*)data, size);
    dp->size_ += size;
    datas_.push_back(dp);
}

void ByteData::appendData(const void *data, size_t size) {
    int len = (int)datas_.size();
    assert(len != 0 && !datas_[len-1]->copy_);
    datas_[len-1]->copy_data_->append((const char*)data, size);
    datas_[len-1]->size_ += size;
}

void ByteData::addFile(const std::string &filepath) {
    int fd = open(filepath.c_str(), O_RDONLY);
    assert(fd > 0);
    struct stat st;
    fstat(fd, &st);
    DataPacket* dp = new DataPacket();
    dp->fd_ = fd;
    dp->copy_ = false;
    dp->size_ += st.st_size;
    dp->zero_copy_data_ = (char*)mmap(NULL, st.st_size, PROT_READ, MAP_PRIVATE, fd, 0);
    assert(dp->zero_copy_data_ != MAP_FAILED);
    datas_.push_back(dp);
}

void ByteData::addFile(int fd, size_t size) {
    assert(fd > 0);
    DataPacket* dp = new DataPacket();
    dp->fd_ = fd;
    dp->copy_ = false;
    dp->size_ += size;
    dp->zero_copy_data_ = (char*)mmap(NULL, size, PROT_READ, MAP_PRIVATE, fd, 0);
    assert(dp->zero_copy_data_ != MAP_FAILED);
    datas_.push_back(dp);
}

ssize_t ByteData::writev(int fd) {
    if(!remain()) return 0;
    std::vector<struct iovec> iovs;
    for(int i = (int)current_index_; i < datas_.size(); ++i) {
        struct iovec iov;
        DataPacket* data = datas_[i];
        if(i == (int)current_index_) {
            iov.iov_base = (void*)(data->data() + offset_);
            iov.iov_len = data->size_ - offset_;
        }else {
            iov.iov_base = (void*)data->data();
            iov.iov_len = data->size_;
        }
        iovs.push_back(iov);
    }

    ssize_t n = net::sockets::writev(fd, (struct iovec*)(&*iovs.begin()), (int)iovs.size());
    if(n > 0) {
        offset_ += n;
        modifyIndexAndOffset();
    }
    return n;
}

bool ByteData::remain() {
    return !(current_index_ == datas_.size() - 1 && offset_ == datas_[datas_.size() - 1]->size_);
}

void ByteData::copyDataIfNeed() {
    if(remain()) {
        for(size_t i = current_index_; i < datas_.size(); ++i) {
            if(i == current_index_) {
                if(datas_[i]->copyIfNeed(offset_)) offset_ = 0;
            }else {
                datas_[i]->copyIfNeed();
            }
        }
    }
}

void ByteData::modifyIndexAndOffset() {
    int size = (int)datas_.size();
    for(int i = (int)current_index_; i < size; ++i) {
        offset_ -= datas_[i]->size_;
        if(offset_ < 0) {
            offset_ += datas_[i]->size_;
            current_index_ = i;
            return;
        }
    }
    current_index_ = size - 1;
    offset_ = (int)datas_[current_index_]->size_;
    return;
}


}
}