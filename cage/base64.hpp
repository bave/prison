#ifndef __BASE64_HPP__
#define __BASE64_HPP__

#include <iostream>
#include <string>
#include <vector>

namespace base64 {
    bool encode(const std::vector<uint8_t>& src, std::string& dst);
    bool decode(const std::string& src, std::vector<uint8_t>& dst);
}

#endif
