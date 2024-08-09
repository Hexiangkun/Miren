#include "http/core/HttpMultipart.h"

namespace Miren {
namespace http {

bool HttpRequestMultipartBody::parse(std::string_view body) {
    size_t i = 0;
    size_t boundary_len = boundary_.size();
    size_t len = body.size();
    std::string name, filename,type,form_file_data;

    bool file_mark = false;
    const char* boundary_s = boundary_.data();

    State state = start_body;

    static constexpr const size_t buf_size = 4096;
    char buffer[buf_size];

    while(i < len) {
        switch (state) {
            case start_body:
            {
                // boundary begin --boundary
                if(i + 1 < len && body[i] == '-' && body[i+1] == '-') {
                    i+=2;
                    state = start_boundary;
                }
                break;
            }
            case start_boundary:
            {
                if(::strncmp(body.data()+i, boundary_s, boundary_len) == 0) {
                    i += boundary_len;
                    // --boundary\r\n
                    if(i+1 < len && body[i] == CR && body[i+1] ==LF) {
                        i += 2;
                        state = end_boundary;
                    }
                        // --boundary--
                    else if(i+1 < len && body[i] == '-' && body[i+1] == '-') {
                        i +=2;
                        state = end_body;
                    }
                    else {
                        //bad body
                        return false;
                    }
                }
                break;
            }
            case end_boundary:
            {
                if(::strncasecmp(body.data()+i, "Content-Disposition", 19) == 0) {
                    i += 19;    //skip Content-Disposition
                    state = start_content_disposition;
                }
                else {
                    return false;
                    //bad body
                }
                break;
            }
            case start_content_disposition:
            {
                i += 13;        //// skip ": form-data; "

                bool start_name_filename = false;
                bool is_name = true;
                while(i < len) {
                    if(i+1 < len && body[i] == CR && body[i+1] == LF) {
                        i += 2;
                        state = end_content_disposition;
                        break;
                    }
                    if(i+1 < len && body[i] == '=' && body[i+1] == '\"') {
                        i += 2;
                        start_name_filename = true;
                    }
                    else if(body[i] == '\"') {
                        start_name_filename = false;
                        is_name = !is_name;
                        i++;
                    }
                    else if(start_name_filename) {
                        if(is_name) {
                            name += body[i++];
                        }
                        else {
                            filename += body[i++];
                        }
                    }
                    else {
                        i++;
                    }
                }
                break;
            }
            case end_content_disposition:
            {
                if(i+1 < len && body[i] == CR && body[i+1] == LF) {
                    i += 2;
                    file_mark = false;
                    state = start_content_data;
                }
                else {
                    if(::strncasecmp(body.data()+i, "Content-Type", 12) == 0) {
                        i += 14;    //skip "Content-Type: "
                        file_mark = true;
                        state = start_content_type;
                    }
                    else {
                        return false;
                        // bad body
                    }
                }
                break;
            }
            case start_content_type:
            {
                while (i < len) {
                    if(i+1 < len && body[i] == CR && body[i+1] == LF) {
                        i += 2;
                        state = end_content_type;
                        break;
                    }
                    else {
                        type += body[i++];
                    }
                }
                break;
            }
            case end_content_type:
            {
                if(i+1 < len && body[i] == CR && body[i+1] == LF) {
                    i += 2;
                    state = start_content_data;
                }
                else {
                    return false;
                    // bad body
                }
                break;
            }

            case start_content_data:
            {
                form_file_data.reserve(buf_size);
                size_t k = 0;
                size_t ix = 1;
                size_t pi = i;

                while(i < len) {
                    if(i+4 < len && body[i] == CR && body[i+1] == LF &&
                            body[i+2] == '-' && body[i+3] == '-' &&
                            ::strncmp(body.data()+i+4, boundary_s, boundary_len) == 0) {
                        if(k != 0) {
                            form_file_data.append(buffer, k);
                        }
                        form_file_data.resize(i-pi);
                        i += 2;
                        state = end_content_data;
                        break;
                    }
                    else {
                        buffer[k++] = body[i++];
                        if(k >= buf_size) {
                            k = 0;
                            ix++;
                            form_file_data.append(buffer, buf_size);
                            form_file_data.reserve(buf_size*ix);
                        }
                    }
                }
                break;
            }

            case end_content_data:
            {
                if(!file_mark) {
                    form_.emplace(name, form_file_data);
                }
                else {
                    auto it = files_.find(name);
                    if(it != files_.end()) {
                        std::shared_ptr<MultipartPart> part = std::make_shared<MultipartPart>(name, filename, type, form_file_data);
                        it->second.emplace_back(part);
                    }
                    else {
                        std::shared_ptr<MultipartPart> part = std::make_shared<MultipartPart>(name, filename, type, form_file_data);
                        files_[name] = std::vector{part};
                    }
                }

                name = filename = type = form_file_data = "";
                file_mark = false;
                state = start_body;
                break;
            }
            case end_body:
            {
                i += 2;
                break;
            }
            default: {
                i++;
                break;
            }
        }
    }
    return true;
}

}
}
