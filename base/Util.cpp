#include "base/Util.h"
#include "base/Base64.h"
#include <string.h>
#include <algorithm>
#include <fstream>

namespace Miren
{
namespace base
{
std::string ToUpper(const std::string& name) {
    std::string rt = name;
    std::transform(rt.begin(), rt.end(), rt.begin(), ::toupper);
    return rt;
}

std::string ToLower(const std::string& name) {
    std::string rt = name;
    std::transform(rt.begin(), rt.end(), rt.begin(), ::tolower);
    return rt;
}

std::string Time2Str(time_t ts, const std::string& format) {
  struct tm tm;
  localtime_r(&ts, &tm);
  char buf[64];
  strftime(buf, sizeof(buf), format.c_str(), &tm);
  return buf;
}

time_t Str2Time(const char* str, const char* format) {
  struct tm t;
  memset(&t, 0, sizeof(t));
  if(!strptime(str, format, &t)) {
      return 0;
  }
  return mktime(&t);
}

bool base64_encode_image(const std::string& path, std::string* res)
{
	// 1. 打开图片文件
	std::ifstream is(path, std::ifstream::in | std::ios::binary);
  if(!is.good()) {
    is.close();
    return false;
  }

	// 2. 计算图片长度
	is.seekg(0, is.end);  //将文件流指针定位到流的末尾
	long int length = is.tellg();
	is.seekg(0, is.beg);  //将文件流指针重新定位到流的开始
	// 3. 创建内存缓存区
	char * buffer = new char[length];
	// 4. 读取图片
	is.read(buffer, length);

	*res = base64::to_base64({buffer, static_cast<size_t>(length)});
	// 到此，图片已经成功的被读取到内存（buffer）中
	delete [] buffer;

	return true;
}

bool base64_decode_image(const std::string& input, const std::string& path) {
	std::ofstream outfile;
	outfile.open(path, std::ofstream::binary);	// This line specifically

  std::string res = base64::from_base64({input.data(), input.size()});
	
	if (outfile.is_open()) {
    outfile.write(res.c_str(), res.size());
  }
	else {  
    return false; 
  }

	outfile.close();
	return true;
}


} // namespace base

} // namespace Miren
