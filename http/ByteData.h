#pragma once
#include "net/Buffer.h"
#include <vector>
#include <unistd.h>
#include <sys/mman.h>
#pragma GCC diagnostic ignored "-Wshadow"
#pragma GCC diagnostic ignored "-Wconversion"
#pragma GCC diagnostic ignored "-Wsign-compare"

namespace Miren {
namespace http {


class DataPacket {
private:
    net::Buffer* copy_data_ = nullptr;
    char* zero_copy_data_ = nullptr;
    
    size_t size_ = 0;
    int fd_ = -1;
    bool copy_ = false;
    
public:
    friend class ByteData;
    ~DataPacket() {
        if(copy_) {
            delete copy_data_;
        }else {
            if(fd_ > 0) {
                munmap(zero_copy_data_, size_);
                close(fd_);
            }
        }
    }
    
    const char* data() const {
        if(!copy_) return zero_copy_data_;
        else return copy_data_->peek();
    }
    
    bool copyIfNeed(size_t offset = 0) {
        if(copy_ || fd_ > 0) return false;
        copy_ = true;
        size_ -= offset;
        copy_data_ = new net::Buffer(size_);
        copy_data_->append(zero_copy_data_ + offset, size_);
        zero_copy_data_ = nullptr;
        return true;
    }
};

class ByteData {
private:
    std::vector<DataPacket*> datas_;
    size_t current_index_ = 0;
    ssize_t offset_ = 0;
    
    void modifyIndexAndOffset();
    
public:
    ~ByteData() {
        for(DataPacket* data : datas_) delete data;
    }
    
    //内部不拷贝，注意不要提前释放数据，如果一次性没有发完的数据需要手动调用CopyDataIfNeed方法对数据进行缓存
    void addDataZeroCopy(const base::StringPiece& data);
    void addDataZeroCopy(const void* data, size_t size);
    
    //内部会存在一次拷贝
    void addDataCopy(const base::StringPiece& data);
    void addDataCopy(const void* data, size_t size);
    void appendData(const void* data, size_t size);
    void addFile(const std::string& filepath);
    void addFile(int fd, size_t size);
 
    ssize_t writev(int fd);
    bool remain();
    void copyDataIfNeed();
};

}
}

