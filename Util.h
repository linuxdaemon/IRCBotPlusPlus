#ifndef IRCBOT_UTIL_H
#define IRCBOT_UTIL_H

#include <string>
#include <vector>

namespace StringUtils {
    void split(const std::string &s, char c, std::vector<std::string> &v);

    bool Base64Encode(std::string &sIn, std::string &sRet, unsigned int uWrap);

    bool Base64Encode(std::string &s, unsigned int uWrap = 0);
};

#endif //IRCBOT_UTIL_H
