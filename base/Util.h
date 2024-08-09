#pragma once

#include <string>
namespace Miren
{
namespace base
{

std::string ToUpper(const std::string& name);

std::string ToLower(const std::string& name);

std::string Time2Str(time_t ts = time(0), const std::string& format = "%Y-%m-%d %H:%M:%S");
time_t Str2Time(const char* str, const char* format = "%Y-%m-%d %H:%M:%S");

bool base64_encode_image(const std::string& path, std::string* res);

bool base64_decode_image(const std::string& input, const std::string& path);


} // namespace base

} // namespace Miren
