#include <iostream>
#include <assert.h>
#include "net/ZlibStream.h" 

using namespace Miren::net;

void zlibInflateDemo() {
    // 压缩前的数据
    std::string inputData = "This is a test string to be compressed and then decompressed.";
    
    // 创建输出缓冲区用于压缩数据
    Buffer compressedData;
    {
        ZlibOutputStream outStream(&compressedData);
        if (!outStream.write(inputData)) {
            std::cerr << "Error during compression." << std::endl;
            return;
        }
        std::cout << outStream.inputBytes() << std::endl;
        std::cout << outStream.outputBytes() << std::endl;
    }

    // 创建输入缓冲区用于解压数据
    Buffer decompressedData;
    {
        ZlibInputStream inStream(&decompressedData);
        if (!inStream.write(&compressedData)) {
            std::cerr << "Error during decompression." << std::endl;
            // return;
        }
    }

    // 打印解压缩后的数据
    std::cout << "Decompressed: " << decompressedData.retrieveAllAsString() << std::endl;
}

int main() {
    zlibInflateDemo();
    return 0;
}
